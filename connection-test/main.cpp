#include <cstdio>

#include <chrono>
#include <memory>

#include <logger/logger.hpp>
#include <robotics/random/random.hpp>
#include <robotics/thread/thread.hpp>

// #include "im920_test.hpp"
// #include "can_debug.hpp"
#include "robo-bus.hpp"

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
