#pragma once

#include "can_servo.hpp"
#include "ikako_robomas_bus.hpp"
#include "rohm_md_bus.hpp"

namespace nhk2024b::robot2 {

class Actuators {
 public:
  struct Config {
    PinName can_1_rd;
    PinName can_1_td;

    PinName can_2_rd;
    PinName can_2_td;
  };

 public:
  ikarashiCAN_mk2 can1;
  ikarashiCAN_mk2 can2;

 public:
  common::CanServoBus can_servo;
  common::RohmMDBus rohm_md;

  Actuators(Config config)
      : can1(config.can_1_rd, config.can_1_td, 0, (int)1E6),
        can2(config.can_2_rd, config.can_2_td, 0, (int)1E6),
        can_servo(can1, 1),
        rohm_md(can1) {}

  void Init() {
    can1.read_start();
    can2.read_start();
  }

  int Read() {
    rohm_md.Read();
    return 0;
  }

  int Send() {
    int errors = 0;
    int status;

    status = can_servo.Send();  // Sends 020H CAN message
    if (status == 0) {
      // logger.Error("CanServoBus::Send failed");
      errors |= 1;
    }

    status = rohm_md.Send();
    if (status != 1) {
      // logger.Error("IkakoRobomasBus::Write failed");
      errors |= 4;
    }

    return -errors;
  }

  void Tick() {
    can1.reset();
    can2.reset();
  }
};
}  // namespace nhk2024b::robot2