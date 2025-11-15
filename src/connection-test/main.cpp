#include "apps/robobus_test.hpp"

int main() {
  using namespace std::chrono_literals;
  printf("# main()\n");

  robotics::system::Random::GetByte();
  robotics::system::SleepFor(20ms);
  robotics::logger::core::Init();

  printf("Starting App...\n");

  auto app = std::make_unique<apps::RoboBusTest>();
  app->Main();

  while (1) {
    robotics::system::SleepFor(1s);
  }
}