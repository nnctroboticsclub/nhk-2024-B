
#include <vector>
#include <iomanip>
#include <string>

#include <robotics/logger/logger.hpp>
#include <robotics/platform/random.hpp>
#include <robotics/platform/thread.hpp>

#include <robotics/network/fep_raw_driver.hpp>
#include <robotics/network/rep.hpp>

#include <robotics/network/ssp/ssp.hpp>
#include <robotics/network/ssp/relay.hpp>
#include <robotics/network/ssp/identity.hpp>
#include <robotics/network/ssp/kv.hpp>
#include <robotics/network/ssp/value_store.hpp>

#include <robotics/controller/float.hpp>

#include "platform.hpp"

//* ####################
//* Services
//* ####################

class NodeInspectorService : public robotics::network::ssp::SSP_Service {
  class NIStream : public robotics::network::Stream<uint8_t, uint8_t> {
    // <SVC> <===> <NIStream>
    robotics::logger::Logger& logger;
    robotics::network::Stream<uint8_t, uint8_t>& svc_stream_;

    struct Message {
      uint8_t from;
      uint8_t data[8];
      size_t size;
    };

    robotics::utils::NoMutexLIFO<Message, 4> rx_queue_;

    void Thread() {
      while (1) {
        if (rx_queue_.Empty()) {
          robotics::system::SleepFor(10ms);
          continue;
        }

        auto msg = rx_queue_.Pop();
        logger.Info("NIStream RX: %d <-- %d", msg.size, msg.from);
        logger.Hex(robotics::logger::core::Level::kInfo, msg.data, msg.size);
        DispatchOnReceive(msg.from, msg.data, msg.size);
      }
    }

   public:
    NIStream(robotics::network::Stream<uint8_t, uint8_t>& svc_stream,
             robotics::logger::Logger& logger)
        : logger(logger), svc_stream_(svc_stream) {
      svc_stream_.OnReceive([this](uint8_t addr, uint8_t* data, size_t len) {
        Message msg;
        std::memcpy(msg.data, data, len);
        msg.size = len;
        msg.from = addr;
        rx_queue_.Push(msg);
      });

      robotics::system::Thread thread;
      thread.SetStackSize(1024);
      thread.Start([this]() { Thread(); });
    }

    void Send(uint8_t to, uint8_t* data, uint32_t length) override {
      if (to == 0) {
        logger.Error("NIStream: Invalid Address: %d", to);
        return;
      }
      logger.Info("NIStream TX: %d --> %d", length, to);
      logger.Hex(robotics::logger::core::Level::kInfo, data, length);
      svc_stream_.Send(to, data, length);
    }
  };

  NIStream ni_stream_;

 public:
  NodeInspectorService(robotics::network::Stream<uint8_t, uint8_t>& stream)
      : SSP_Service(stream, 0x0400, "nodeinspect.svc.nw",
                    "\x1b[32mNodeInspectorService\x1b[m"),
        ni_stream_(*this, logger) {
    robotics::node::NodeInspector::RegisterStream(
        std::shared_ptr<robotics::network::Stream<uint8_t, uint8_t>>(
            &ni_stream_));
  }
};

class TestService : public robotics::network::ssp::SSP_Service {
 public:
  TestService(robotics::network::Stream<uint8_t, uint8_t>& stream)
      : SSP_Service(stream, 0xffff, "test.svc.nw",
                    "\x1b[32mTestService\x1b[m") {
    OnReceive([this](uint8_t addr, uint8_t* data, size_t len) {
      logger.Info("RX: %d", addr);
      logger.Hex(robotics::logger::core::Level::kInfo, data, len);
    });
  }
};

namespace f_route {

namespace {
robotics::logger::Logger logger{"frt.nw", "\x1b[1;36mFRt\x1b[m"};
robotics::logger::Logger rx_logger{"rx.frt.nw",
                                   "\x1b[1;36mFRt - \x1b[34mRX\x1b[m"};
robotics::logger::Logger tx_logger{"tx.frt.nw",
                                   "\x1b[1;36mFRt - \x1b[32mTX\x1b[m"};
}  // namespace

struct Action {
  enum class Type { kFindNewHopTo, kReportDiedPacket, kAdvertiseSelf };
  Type type;

  uint8_t report_to;

  static Action FindNewHopTo() { return {Type::kFindNewHopTo, 0}; }

  static Action ReportDiedPacket(uint8_t report_to) {
    return {Type::kReportDiedPacket, report_to};
  }

  static Action AdvertiseSelf(uint8_t report_to) {
    return {Type::kAdvertiseSelf, report_to};
  }
};

struct AddrRecords {
  uint8_t records[32];
  size_t recorded;

  bool Recorded(uint8_t rec) {
    for (size_t i = 0; i < recorded; i++) {
      if (records[i] == rec) {
        return true;
      }
    }

    return false;
  }

  void Add(uint8_t rec) {
    if (Recorded(rec)) return;

    records[recorded++] = rec;
  }
};

struct Packet {
  uint8_t using_hop;

  uint8_t life;
  uint8_t from;
  uint8_t goal;
  uint8_t flags;

  uint8_t data[32];
  uint32_t size;
};

class ProtoFRoute : public robotics::network::Stream<uint8_t, uint8_t> {
  const uint8_t kFlagsDetectDevices = 0x01;
  const uint8_t kFlagsNewDevice = 0x02;
  const uint8_t kFlagsPacketDied = 0x03;

  // queue
  robotics::utils::NoMutexLIFO<Action, 4> remaining_actions_;
  robotics::utils::NoMutexLIFO<Packet, 4> remaining_packets_;

  // avoid bidirectional loop
  AddrRecords incoming_data_from_;

  // thread
  robotics::system::Thread thread;

  // connection state
  robotics::network::Stream<uint8_t, uint8_t>& upper_stream_;
  uint8_t next_hop_ = 0;

  // address
  uint8_t broadcast_addr_;
  uint8_t self_addr_;

  void ProcessAction(Action& action) {
    switch (action.type) {
      case Action::Type::kFindNewHopTo: {
        if (next_hop_ != 0) {
          return;
        }

        logger.Info("Find New Hop To");
        SendEx(broadcast_addr_, nullptr, 0, kFlagsDetectDevices, self_addr_);

        return;
      }

      case Action::Type::kReportDiedPacket: {
        logger.Info("Report Died Packet to %d", action.report_to);
        SendEx(action.report_to, &self_addr_, 1, kFlagsPacketDied, self_addr_);
        return;
      }

      case Action::Type::kAdvertiseSelf: {
        logger.Info("Advertise Self to %d", action.report_to);
        SendEx(action.report_to, nullptr, 0, kFlagsNewDevice, self_addr_);
        return;
      }
    }
  }

  void DoSend(Packet* packet) {
    static char buffer[32];
    buffer[0] = packet->life;
    buffer[1] = packet->from;
    buffer[2] = packet->goal;
    buffer[3] = packet->flags;
    if (packet->data != nullptr) {
      std::memcpy(buffer + 4, packet->data, packet->size);
    }

    tx_logger.Info("Send Data = %p (%d B) --> %d (l%d %d --> %d f%#02x)",
                   packet->data, packet->size, packet->goal, packet->life,
                   packet->from, packet->goal, packet->flags);
    tx_logger.Hex(robotics::logger::core::Level::kInfo, packet->data,
                  packet->size);

    if (packet->using_hop == 0 && next_hop_ != 0) {
      packet->using_hop = next_hop_;
    }

    upper_stream_.Send(packet->using_hop, (uint8_t*)buffer, packet->size + 4);
  }

  void Thread() {
    while (1) {
      if (remaining_actions_.Empty() && remaining_packets_.Empty()) {
        robotics::system::SleepFor(10ms);
        continue;
      }

      while (!remaining_actions_.Empty()) {
        auto action = remaining_actions_.Pop();
        ProcessAction(action);
      }

      for (size_t i = 0; i < remaining_packets_.Size(); i++) {
        auto packet = remaining_packets_.Pop();
        DoSend(&packet);
      }
    }
  }

  bool ProcessFlags(uint8_t from, uint8_t flags, uint8_t* data, uint32_t size) {
    if (flags == kFlagsDetectDevices) {
      for (uint i = 0; i < size; i++) {
        if (data[i] == self_addr_) return true;
      }

      remaining_actions_.Push(Action::AdvertiseSelf(from));
    } else if (flags == kFlagsNewDevice) {
      if (incoming_data_from_.Recorded(from)) return true;
      logger.Info("Using Hop: %d", from);
      next_hop_ = from;
    } else if (flags == kFlagsPacketDied) {
      logger.Info("Packet Died from %d", from);
      if (from == next_hop_) {
        next_hop_ = 0;
        remaining_actions_.Push(Action::FindNewHopTo());
      }
    } else {
      return false;
    }

    return true;
  }

 public:
  ProtoFRoute(robotics::network::Stream<uint8_t, uint8_t>& upper_stream,
              uint8_t broadcast_addr, uint8_t self_addr)
      : upper_stream_(upper_stream),
        broadcast_addr_(broadcast_addr),
        self_addr_(self_addr) {
    incoming_data_from_ = {0, 0};
    remaining_actions_.Push(Action::FindNewHopTo());

    upper_stream_.OnReceive([this](uint8_t from, uint8_t* data, uint32_t size) {
      auto ptr = data;

      auto life = *ptr++;
      auto original_from = *ptr++;
      auto goal = *ptr++;
      auto flags = *ptr++;

      auto payload = ptr;
      auto payload_size = size - 4;

      rx_logger.Info("Recv Data = %p (%d B) (l%d %d --> %d f%#02x)", payload,
                     payload_size, life, original_from, goal, flags);
      rx_logger.Hex(robotics::logger::core::Level::kInfo, payload,
                    payload_size);

      if (ProcessFlags(from, flags, payload, payload_size)) {
        return;
      }

      incoming_data_from_.Add(from);

      if (life == 0) {
        remaining_actions_.Push(Action::ReportDiedPacket(from));
        return;
      }

      if (goal == self_addr_) {
        DispatchOnReceive(original_from, payload, payload_size);
      } else {
        SendEx(goal, payload, payload_size, flags, original_from);
      }

      if (from == next_hop_) {
        next_hop_ = 0;
        remaining_actions_.Push({Action::Type::kFindNewHopTo, 0});
      }
    });

    thread.SetStackSize(1024);
  }

  void Start() {
    thread.Start([this]() { Thread(); });
  }

  void SendEx(uint8_t to, uint8_t* data, uint32_t length, uint8_t flags,
              uint8_t from) {
    Packet packet;
    packet.using_hop = to == broadcast_addr_ ? broadcast_addr_
                       : next_hop_ == 0      ? to
                                             : next_hop_;
    packet.life = 0x80;
    packet.from = from;
    packet.goal = to;
    packet.flags = flags;
    packet.size = length;
    std::memcpy(packet.data, data, length);

    remaining_packets_.Push(packet);
  }
  void Send(uint8_t to, uint8_t* data, uint32_t length) override {
    SendEx(to, data, length, 0, self_addr_);
  }
};
}  // namespace f_route

//* ####################
//* App
//* ####################

using robotics::logger::core::Level;
using namespace robotics::network::ssp;
class Network {
  robotics::logger::Logger logger{"nw.app", "\x1b[1;32mNetwork\x1b[m"};
#if defined(__TEST_ON_HOST__)
  robotics::network::UARTStream uart_stream_;
#else
  robotics::network::UARTStream uart_stream_{PB_6, PA_10, 9600};
#endif

  robotics::network::FEP_RawDriver fep_;
  robotics::network::ReliableFEPProtocol rep_;
  f_route::ProtoFRoute f_route_;
  robotics::network::SerialServiceProtocol ssp_;

  void SetupFEP() {
    auto fep_conf = platform::GetFEPConfiguration();

    auto fep_address = fep_conf.address;
    auto group_address = 0xF0;
    logger.Info("# Setup FEP");
    logger.Info("- Addr         : %d", fep_address);
    logger.Info("- Group Address: %d", group_address);

    {
      auto res = fep_.InitAllRegister();
      if (!res.IsOk()) {
        logger.Error("FAILED!!!: %s", res.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto result = fep_.Reset();
      if (!result.IsOk()) {
        logger.Error("FAILED!!!: %s", result.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto res = fep_.SetRegister(0, fep_address);
      if (!res.IsOk()) {
        logger.Error("FAILED!!!: %s", res.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto res = fep_.SetRegister(1, group_address);
      if (!res.IsOk()) {
        logger.Error("FAILED!!!: %s", res.UnwrapError().c_str());
        while (1);
      }
    }

    /* {
      auto res = fep_.SetRegister(11, 0x80);
      if (!res.IsOk()) {
        logger.Error("FAILED!!!: %s", res.UnwrapError().c_str());
        while (1);
      }
    } */

    {
      auto res = fep_.SetRegister(18, 0x8D);
      if (!res.IsOk()) {
        logger.Error("FAILED!!!: %s", res.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto res = fep_.Reset();
      if (!res.IsOk()) {
        logger.Error("FAILED!!!: %s", res.UnwrapError().c_str());
        while (1);
      }
    }
  }

 public:
  struct {
    robotics::network::ssp::RelayService* relay;
    robotics::network::ssp::IdentitiyService* ident;
    TestService* test;
    robotics::network::ssp::ValueStoreService* value_store;
    NodeInspectorService* node_inspector;
  } svc;

 public:
  Network()
      : fep_(uart_stream_),
        rep_{fep_},
        f_route_(rep_, 0xF0, platform::GetSelfAddress()),
        ssp_(f_route_) {}

  void Main() {
    using namespace std::chrono_literals;

    robotics::logger::SuppressLogger("sr.fep.nw");
    robotics::logger::SuppressLogger("st.fep.nw");

    robotics::logger::SuppressLogger("tx.rep.nw");
    robotics::logger::SuppressLogger("rx.rep.nw");

    SetupFEP();

    auto device_name = platform::GetDeviceName();

    svc = {
        .relay = ssp_.RegisterService<robotics::network::ssp::RelayService>(),
        .ident = ssp_.RegisterService<robotics::network::ssp::IdentitiyService>(
            device_name),
        .test = ssp_.RegisterService<TestService>(),
        .value_store =
            ssp_.RegisterService<robotics::network::ssp::ValueStoreService>(),
        .node_inspector = ssp_.RegisterService<NodeInspectorService>(),
    };

    auto dest_address = platform::GetRemoteTestAddress();
    logger.Info("Dest Address: %d\n", dest_address);

    f_route_.Start();
  }
};

class App {
  robotics::logger::Logger logger{"app", "\x1b[1;32mApp\x1b[m"};
  Network network;

  controller::Float ctrl_a;

 public:
  App() : ctrl_a(0x00) {}
  void Main() {
    network.Main();

    network.svc.value_store->AddController(ctrl_a);
    ctrl_a.SetChangeCallback([this](float v) { logger.Info("CtrlA: %f", v); });

    /*auto dest_address = platform::GetRemoteTestAddress();

    mbed::InterruptIn watch_to(PC_7);
    watch_to.rise([this, dest_address]() {
      logger.Info("WatchPin Rised!");
      network.svc.test->Send(0x80, (uint8_t*)"\x80hello", 5);
    }); */

    while (1) {
      robotics::system::SleepFor(3600s);
    }
  }
};

int main() {
  using namespace std::chrono_literals;

  printf("# main()\n");

  robotics::system::Random::GetByte();
  robotics::system::SleepFor(20ms);

  robotics::logger::Init();

  App* app = new App();

  app->Main();

  return 0;
}
