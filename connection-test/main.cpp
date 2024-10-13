#include <cstdio>

#include <chrono>
#include <memory>

#include <logger/logger.hpp>
#include <robotics/random/random.hpp>
#include <robotics/thread/thread.hpp>

// #include "im920_test.hpp"
// #include "can_debug.hpp"
#include "robobus/robo-bus.hpp"

using robobus::robobus::RoboBus;
using robobus::types::DeviceID;

class RoboBusTest {
  static inline robotics::logger::Logger logger_{"test.robo-bus.nw",
                                                 "RoboBusTest"};

 public:
  [[noreturn]]
  void Main() const {
    using enum platform::Mode;

    auto mode = platform::GetMode();

    DeviceID device_id{0};
    DeviceID remote_device_id{0};
    bool is_motherboard = false;
    switch (mode) {
      case kDevice1:
        device_id = DeviceID(0);
        remote_device_id = DeviceID(1);
        is_motherboard = true;
        break;
      case kDevice2:
        device_id = DeviceID(1);
        remote_device_id = DeviceID(0);
        is_motherboard = false;
        break;
    }

    logger_.Info("Device ID   : %d", device_id.GetDeviceID());
    logger_.Info("Motherboard?: %d", is_motherboard);

    auto simple_can =
        std::make_unique<robotics::network::SimpleCAN>(PB_8, PB_9, 1E6);
    simple_can->SetCANExtended(true);  // RoboBus uses extended ID

    auto can = static_cast<std::unique_ptr<robotics::network::CANBase>>(
        std::move(simple_can));

    auto robobus = std::make_unique<RoboBus>(
        DeviceID(platform::GetSelfAddress()), is_motherboard, std::move(can));

    using namespace std::chrono_literals;
    while (true) {
      robobus->Test(remote_device_id);
      robotics::system::SleepFor(500ms);
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

  auto app = std::make_unique<RoboBusTest>();
  app->Main();
}
