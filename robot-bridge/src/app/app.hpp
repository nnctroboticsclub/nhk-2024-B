#pragma once

#include "can_servo.hpp"
#include "ikako_robomas_bus.hpp"

namespace nhk2024b::robot2 {
class Actuators {
 public:
  struct Config {
    PinName can_1_rd;
    PinName can_1_td;
  };

 private:
  ikarashiCAN_mk2 can;

 public:
  common::CanServoBus can_servo;
  common::IkakoRobomasBus ikako_robomas;

  Actuators(Config config)
      : can(config.can_1_rd, config.can_1_td, (int)500E3),
        can_servo(can, 2),
        ikako_robomas(can) {}
};
}  // namespace nhk2024b::robot2