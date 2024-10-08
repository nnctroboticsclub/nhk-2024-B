
#include <vector>
#include <iomanip>
#include <string>

#include <logger/logger.hpp>
#include <robotics/random/random.hpp>
#include <robotics/thread/thread.hpp>
#include <robotics/driver/dout.hpp>

#include <robotics/network/simple_can.hpp>

#include <ssp/ssp.hpp>
#include <ssp/value_store.hpp>

#include "platform.hpp"

#include <srobo2/com/im920_srobo1.hpp>
#include <srobo2/com/mbed_cstream.hpp>
#include <srobo2/timer/mbed_timer.hpp>

//* ####################
//* Services
//* ####################

/* template <typename Context>
class NodeInspectorService
    : public robotics::network::ssp::SSP_Service<Context> {
  class NIStream : public robotics::network::Stream<uint8_t, Context> {
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
      : SSP_Service<Context>(stream, 0x11, "nodeinspect.svc.nw",
                             "\x1b[32mNodeInspectorService\x1b[m"),
        ni_stream_(*this, logger) {
    robotics::node::NodeInspector::RegisterStream(
        std::shared_ptr<robotics::network::Stream<uint8_t, uint8_t>>(
            &ni_stream_));
  }
};

template <typename Context>
class TestService : public robotics::network::ssp::SSP_Service<Context> {
 public:
  TestService(robotics::network::Stream<uint8_t, uint8_t>& stream)
      : SSP_Service<Context>(stream, 0xff, "test.svc.nw",
                             "\x1b[32mTestService\x1b[m") {
    OnReceive([this](Context addr, uint8_t* data, size_t len) {
      logger.Info("RX: %d", addr);
      logger.Hex(robotics::logger::core::Level::kInfo, data, len);
    });
  }
}; */

//* ####################
//* App
//* ####################

using robotics::logger::core::Level;
using namespace robotics::network::ssp;

template <typename Context>
class Network {
  robotics::logger::Logger logger{"nw.app", "\x1b[1;32m Network \x1b[m"};

 public:
  void Main() {
    using namespace std::chrono_literals;

    robotics::logger::SuppressLogger("rxp.fep.nw");
    robotics::logger::SuppressLogger("st.fep.nw");
    robotics::logger::SuppressLogger("sr.fep.nw");

    robotics::logger::SuppressLogger("tx.rep.nw");
    robotics::logger::SuppressLogger("rx.rep.nw");

    printf("AA4\n");
    auto im920_uart_ =
        std::make_shared<mbed::UnbufferedSerial>(PA_9, PA_10, 19200);

    printf("AA5\n");
    srobo2::com::UARTCStreamRx rx_{im920_uart_};
    srobo2::com::UARTCStreamTx tx_{im920_uart_};
    srobo2::timer::MBedTimer timer_;

    printf("AA6\n");
    srobo2::com::CIM920 cim920_{tx_.GetTx(), rx_.GetRx(), timer_.GetTime()};
    printf("AA7\n");
    srobo2::com::IM920_SRobo1 im920_{&cim920_};
    robotics::network::SerialServiceProtocol<uint16_t, bool> ssp_{im920_};

    robotics::Node<float> ctrl_a;
    auto value_store = ssp_.RegisterService<
        robotics::network::ssp::ValueStoreService<Context, bool>>();

    ctrl_a.SetChangeCallback([this](float v) { logger.Info("CtrlA: %f", v); });
    value_store->AddController(0, 1, ctrl_a);

    while (true) {
      robotics::system::SleepFor(1s);
    }
  }
};

class App {
  Network<uint16_t> network;

 public:
  App() = default;
  void Main() { network.Main(); }
};

struct CanMessageData {
  static uint8_t last_line;

  bool is_invalidated = true;
  uint8_t line = last_line;
  std::array<std::uint8_t, 8> data = {};
  uint8_t length = 0;

  int rx_count = 0;

  static void AddLine() { last_line++; }
};

uint8_t CanMessageData::last_line = 0;

class CanDebug {
  const int kHeaderLines = 3;

  robotics::network::SimpleCAN can_{PA_11, PA_12, (int)1E6};

  std::unordered_map<uint32_t, CanMessageData> messages_;
  uint32_t messages_count_ = 0;
  int tick_ = 0;
  int last_failed_tick_ = 0;

  bool printf_lock_ = false;

  void InitScreen() const {
    printf("\x1b[2J\x1b[1;1H");
    printf("\x1b[?25l");
  }

  void UpdateScreen() {
    while (printf_lock_) {
      robotics::system::SleepFor(1ms);
    }
    printf_lock_ = true;

    for (auto&& [id, data] : messages_) {
      if (!data.is_invalidated) {
        continue;
      }

      printf("\x1b[%d;1H", kHeaderLines + 1 + data.line);
      printf("%8d] %08lX (%5u):", tick_, id, data.rx_count);
      for (size_t i = 0; i < data.length; i++) {
        printf(" %02X", static_cast<uint8_t>(data.data[i]));
      }
      printf("\x1b[0K\n");

      data.is_invalidated = false;
    }

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
    printf("Messages: %5ld\x1b[0K\n", messages_count_);
    printf("Last Failed: %5d\x1b[0K\n", last_failed_tick_);
    printf_lock_ = false;
  }

  void Init() {
    InitScreen();
    can_.OnRx([this](uint32_t id, std::vector<uint8_t> data) {
      if (messages_.find(id) == messages_.end()) {
        messages_[id] = CanMessageData();
        messages_count_++;
        CanMessageData::AddLine();
      }

      auto&& msg = messages_[id];
      msg.is_invalidated = true;
      msg.rx_count++;
      msg.length = static_cast<uint8_t>(data.size());
      std::copy(data.begin(), data.end(), msg.data.begin());
    });
  }

  void TestSend() {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    auto ret = can_.Send(0x400, data);

    if (ret != 1) {
      last_failed_tick_ = tick_;
    }
  }

 public:
  [[noreturn]]
  void Main() {
    Init();
    can_.Init();

    while (true) {
      UpdateScreen();
      ShowHeader();
      tick_++;
      robotics::system::SleepFor(100ms);

      TestSend();
    }
  }
};

class IM920Test {
  [[noreturn]]
  void Task() const {
    auto logger = robotics::logger::Logger("main", "Main");

    auto uart = std::make_shared<mbed::UnbufferedSerial>(PA_9, PA_10, 19200);
    auto tx = std::make_shared<srobo2::com::UARTCStreamTx>(uart);
    auto rx = std::make_shared<srobo2::com::UARTCStreamRx>(uart);
    auto timer = std::make_shared<srobo2::timer::MBedTimer>();

    auto im920 = std::make_shared<srobo2::com::CIM920>(tx->GetTx(), rx->GetRx(),
                                                       timer->GetTime());

    auto node_number = im920->GetNodeNumber(1.0);
    logger.Info("Node Number: %d", node_number);

    uint16_t remote = 3 - node_number;

    auto version = im920->GetVersion(1.0);
    logger.Info("Version: %s", version.c_str());

    im920->OnData([&logger](uint16_t from, uint8_t* data, size_t len) {
      logger.Info("OnData: from %d", from);
      logger.Hex(robotics::logger::core::Level::kInfo, data, len);
    });

    while (true) {
      im920->Send(remote, (uint8_t*)"Hello", 5, 1.0);
      robotics::system::SleepFor(1s);
    }
  }

 public:
  [[noreturn]]
  void Main() const {
    auto thread = std::make_unique<robotics::system::Thread>();
    thread->SetStackSize(8192);
    thread->SetThreadName("App");
    thread->Start([this]() { this->Task(); });

    while (true) {
      robotics::system::SleepFor(100s);
    }
  }
};

int main() {
  using namespace std::chrono_literals;

  printf("# main()\n");

  robotics::system::Random::GetByte();
  robotics::system::SleepFor(20ms);
  robotics::logger::core::Init();

  printf("Starting App...\n");

  auto app = std::make_unique<App>();
  app->Main();

  while (true) {
    robotics::system::SleepFor(3600s);
  }

  return 0;
}
