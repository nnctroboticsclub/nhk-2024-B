
#include <mbed.h>

#include <sstream>
#include <vector>
#include <iomanip>
#include <string>
#include <queue>
#include <variant>
#include <functional>
#include <unordered_map>

#include <ctype.h>

#include <robotics/network/stream.hpp>
#include <robotics/logger/logger.hpp>
#include <robotics/platform/random.hpp>
#include <robotics/types/result.hpp>

#include <robotics/network/fep_raw_driver.hpp>
#include <robotics/network/rep.hpp>
#include <robotics/network/ssp.hpp>
#include <mbed-robotics/uart_stream.hpp>

class EchoService : public robotics::network::SSP_Service {
 public:
  EchoService(robotics::network::Stream<uint8_t, uint8_t>& stream)
      : SSP_Service(stream, 0x0001) {
    OnReceive([this](uint8_t addr, uint8_t* data, size_t len) {
      robotics::logger::Log(robotics::logger::Level::kInfo, "[Echo] RX: %d",
                            addr);
      robotics::logger::LogHex(robotics::logger::Level::kInfo, data, len);
      robotics::logger::Log(robotics::logger::Level::kInfo, "[Echo] TX: %d",
                            addr);
      robotics::logger::LogHex(robotics::logger::Level::kInfo, data, len);
      Send(addr, data, len);
    });
  }
};
class Echo2Service : public robotics::network::SSP_Service {
 public:
  Echo2Service(robotics::network::Stream<uint8_t, uint8_t>& stream)
      : SSP_Service(stream, 0x0002) {
    OnReceive([this, &stream](uint8_t addr, uint8_t* data, size_t len) {
      robotics::logger::Log(robotics::logger::Level::kInfo, "[Echo2] RX: %d",
                            addr);
      robotics::logger::LogHex(robotics::logger::Level::kInfo, data, len);

      if (len > 0) {
        addr = data[0];
        data += 1;
        len -= 1;
        robotics::logger::Log(robotics::logger::Level::kInfo, "[Echo2] TX: %d",
                              addr);
        robotics::logger::LogHex(robotics::logger::Level::kInfo, data, len);
        stream.Send(addr, data, len);
      }
    });
  }
};

using robotics::logger::Level;
class App {
  robotics::network::UARTStream uart_stream_;
  robotics::network::FEP_RawDriver fep_;
  robotics::network::ReliableFEPProtocol rep_;
  robotics::network::SerialServiceProtocol ssp_;

  void SetupFEP(int mode) {
    auto fep_address = mode == 1 ? 2 : 1;
    auto group_address = 0xF0;
    robotics::logger::Log(Level::kInfo, "# Setup FEP");
    robotics::logger::Log(Level::kInfo, "- Mode         : %d", mode);
    robotics::logger::Log(Level::kInfo, "- Addr         : %d", fep_address);
    robotics::logger::Log(Level::kInfo, "- Group Address: %d", group_address);

    {
      auto res = fep_.InitAllRegister();
      if (!res.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              res.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto result = fep_.Reset();
      if (!result.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              result.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto res = fep_.SetRegister(0, fep_address);
      if (!res.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              res.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto res = fep_.SetRegister(1, group_address);
      if (!res.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              res.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto res = fep_.SetRegister(18, 0x8D);
      if (!res.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              res.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto res = fep_.Reset();
      if (!res.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              res.UnwrapError().c_str());
        while (1);
      }
    }
  }

  int GetMode() {
    mbed::DigitalIn mode(PA_9);

    return mode.read() ? 1 : 0;
  }

 public:
  App()
      : uart_stream_{PB_6, PA_10, 9600},
        fep_(uart_stream_),
        rep_{fep_},
        ssp_(rep_) {}

  void Main() {
    auto mode = GetMode();

    SetupFEP(mode);

    ssp_.RegisterService<EchoService>();
    ssp_.RegisterService<Echo2Service>();

    auto dest_address = mode == 1 ? 1 : 2;
    robotics::logger::Log(Level::kInfo, "[App] Dest Address: %d\n",
                          dest_address);

    std::vector<uint8_t> data = {'A', 'B', 'C'};

    while (1) {
      ThisThread::sleep_for(3600s);
    }
  }
};

int main() {
  printf("# main()\n");
  printf("- Build info\n");
  printf("  - Date: %s\n", __DATE__);
  printf("  - Time: %s\n", __TIME__);

  robotics::system::Random::GetByte();
  ThisThread::sleep_for(20ms);

  robotics::logger::Init();

  App* app = new App();

  app->Main();

  return 0;
}
