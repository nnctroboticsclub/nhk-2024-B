
#include <vector>
#include <iomanip>
#include <string>

#include <robotics/logger/logger.hpp>
#include <robotics/platform/random.hpp>
#include <robotics/platform/thread.hpp>
#include <robotics/platform/dout.hpp>

#include <robotics/network/fep/fep_driver.hpp>
#include <robotics/network/rep.hpp>
#include <robotics/network/froute/froute.hpp>
#include <mbed-robotics/simple_can.hpp>

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
      : SSP_Service(stream, 0x11, "nodeinspect.svc.nw",
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
      : SSP_Service(stream, 0xff, "test.svc.nw", "\x1b[32mTestService\x1b[m") {
    OnReceive([this](uint8_t addr, uint8_t* data, size_t len) {
      logger.Info("RX: %d", addr);
      logger.Hex(robotics::logger::core::Level::kInfo, data, len);
    });
  }
};

//* ####################
//* App
//* ####################

using robotics::logger::core::Level;
using namespace robotics::network::ssp;
class Network {
  robotics::logger::Logger logger{"nw.app", "\x1b[1;32m Network \x1b[m"};
#if defined(__TEST_ON_HOST__)
  robotics::network::UARTStream uart_stream_;
#else
  robotics::network::UARTStream uart_stream_{PB_6, PA_10, 115200};
#endif
  robotics::driver::Dout fep_rst{PC_9};
  robotics::driver::Dout fep_ini{PC_8};

  robotics::network::fep::FEPDriver fep_;
  robotics::network::fep::RawFEP_NoTxRet raw_fep_;
  // robotics::network::ReliableFEPProtocol rep_;
  robotics::network::froute::ProtoFRoute f_route_;
  robotics::network::SerialServiceProtocol ssp_;

  void SetupFEP() {
    using robotics::network::fep::FEPBaudrate;
    using robotics::network::fep::FEPBaudrateValue;

    auto fep_conf = platform::GetFEPConfiguration();

    auto fep_address = fep_conf.address;
    auto group_address = 0xF0;
    logger.Info("# Setup FEP");
    logger.Info("- Addr         : %d", fep_address);
    logger.Info("- Group Address: %d", group_address);

    this->fep_.AddConfiguredRegister(0, fep_address);
    this->fep_.AddConfiguredRegister(1, group_address);

    //
    this->fep_.AddConfiguredRegister(11, 0x0A);
    this->fep_.AddConfiguredRegister(18, 0x8F);
    this->fep_.ConfigureBaudrate(FEPBaudrate(FEPBaudrateValue::k115200));

    this->fep_.Init();
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
      : fep_(uart_stream_, fep_rst, fep_ini),
        raw_fep_(fep_.GetFEP_NoTxRet()),
        // rep_(fep_.GetFEP()),
        f_route_{raw_fep_, 0xF0, platform::GetSelfAddress()},
        ssp_(f_route_) {}

  void Main() {
    using namespace std::chrono_literals;

    /* robotics::logger::SuppressLogger("rx.frt.nw");
    robotics::logger::SuppressLogger("tx.frt.nw"); */

    robotics::logger::SuppressLogger("rxp.fep.nw");
    robotics::logger::SuppressLogger("st.fep.nw");
    robotics::logger::SuppressLogger("sr.fep.nw");

    robotics::logger::SuppressLogger("tx.rep.nw");
    robotics::logger::SuppressLogger("rx.rep.nw");

    SetupFEP();
    /* while (1) robotics::system::SleepFor(2s);
    return; */

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
    logger.Info("Dest Address: %d", dest_address);

    f_route_.Start();
  }
};

class App {
  robotics::logger::Logger logger{"app", "\x1b[3;32m   App   \x1b[m"};
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

class CanDebug {
  robotics::network::SimpleCAN can_{PA_11, PA_12, (int)1E6};
  std::unordered_map<uint32_t, uint8_t> id_to_line_y_;
  std::unordered_map<uint32_t, uint16_t> count_per_id_;
  uint32_t messages_count_ = 0;
  int tick_ = 0;
  int last_failed_tick_ = 0;

  const int kHeaderLines = 3;

  bool printf_lock_ = false;

  void InitScreen() {
    printf("\x1b[2J\x1b[1;1H");
    printf("\x1b[?25l");
  }

  void DrawLine(uint8_t y, uint32_t id, uint8_t* data, size_t len) {
    while (printf_lock_) {
      robotics::system::SleepFor(1ms);
    }

    printf_lock_ = true;
    printf("\x1b[%d;1H", kHeaderLines + y);
    printf("%08X (%5d):", id, count_per_id_[id]);
    for (size_t i = 0; i < len; i++) {
      printf(" %02X", data[i]);
    }
    printf("\x1b[0K\n");
    printf_lock_ = false;
  }

  void ShowHeader() {
    while (printf_lock_) {
      robotics::system::SleepFor(1ms);
    }

    printf_lock_ = true;
    printf("\x1b[1;1H");
    printf("\x1b[2K");
    printf("Tick: %5d\x1b[0K\n", tick_);
    printf("Messages: %5d\x1b[0K\n", messages_count_);
    printf("Last Failed: %5d\x1b[0K\n", last_failed_tick_);
    printf_lock_ = false;
  }

  void Init() {
    InitScreen();
    can_.OnRx([this](uint32_t id, std::vector<uint8_t> data) {
      if (id_to_line_y_.find(id) == id_to_line_y_.end()) {
        id_to_line_y_[id] = id_to_line_y_.size() + 1;
      }
      if (count_per_id_.find(id) == count_per_id_.end()) {
        count_per_id_[id] = 0;
      }

      count_per_id_[id]++;
      messages_count_++;

      DrawLine(id_to_line_y_[id], id, data.data(), data.size());
    });
  }

  void TestSend() {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    auto ret = can_.Send(0x3f0, data);

    if (ret != 1) {
      last_failed_tick_ = tick_;
    }
  }

 public:
  void Main() {
    Init();
    can_.Init();

    while (1) {
      ShowHeader();
      tick_++;
      robotics::system::SleepFor(100ms);

      TestSend();
    }
  }
};

int main() {
  using namespace std::chrono_literals;

  printf("# main()\n");

  robotics::system::Random::GetByte();
  robotics::system::SleepFor(20ms);

  robotics::logger::Init();

  auto app = new CanDebug();

  app->Main();

  return 0;
}
