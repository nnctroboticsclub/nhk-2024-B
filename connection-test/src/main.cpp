
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
#include <robotics/network/ssp/kv.hpp>

#include "platform.hpp"

//* ####################
//* Services
//* ####################

class IdentitiyService : public robotics::network::ssp::KVService {
 public:
  IdentitiyService(robotics::network::Stream<uint8_t, uint8_t>& stream)
      : KVService(stream, 0x0000, "id.svc.nw",
                  "\x1b[32mIdentitiyService\x1b[m") {
    using robotics::network::ssp::KVPacket;
    this->OnKVRequested(0x0000,
                        []() { return (KVPacket){(uint8_t*)"test", 4}; });
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
  robotics::network::SerialServiceProtocol ssp_;

  void SetupFEP(platform::Mode mode) {
    auto fep_address = mode == platform::Mode::kDevice1 ? 2 : 1;
    auto group_address = 0xF0;
    logger.Info("# Setup FEP");
    logger.Info("- Mode         : %d", mode);
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
    IdentitiyService* ident;
    TestService* test;
  } svc;

 public:
  Network() : fep_(uart_stream_), rep_{fep_}, ssp_(rep_) {}

  void Main() {
    using namespace std::chrono_literals;

    SetupFEP(platform::GetMode());

    this->svc = {
        .relay = ssp_.RegisterService<robotics::network::ssp::RelayService>(),
        .ident = ssp_.RegisterService<IdentitiyService>(),
        .test = ssp_.RegisterService<TestService>()};

    auto dest_address = platform::GetRemoteTestAddress(platform::GetMode());
    logger.Info("Dest Address: %d\n", dest_address);
  }
};

class App {
  robotics::logger::Logger logger{"app", "\x1b[1;32mApp\x1b[m"};
  Network network;

 public:
  void Main() {
    robotics::logger::SuppressLogger("sr.fep.nw");
    robotics::logger::SuppressLogger("st.fep.nw");

    network.Main();

    auto dest_address = platform::GetRemoteTestAddress(platform::GetMode());

    mbed::InterruptIn watch_to(PC_7);
    watch_to.rise([this, dest_address]() {
      logger.Info("WatchPin Rised!");
      this->network.svc.test->Send(0x80, (uint8_t*)"\x80hello", 5);
    });

    while (1) {
      robotics::system::SleepFor(3600s);
    }
  }
};

int main() {
  using namespace std::chrono_literals;

  printf("# main()\n");
  printf("- Build info\n");
  printf("  - Date: %s\n", __DATE__);
  printf("  - Time: %s\n", __TIME__);
  printf("  - sizeof(App) = %d\n", sizeof(App));

  robotics::system::Random::GetByte();
  robotics::system::SleepFor(20ms);

  robotics::logger::Init();

  App* app = new App();

  app->Main();

  return 0;
}
