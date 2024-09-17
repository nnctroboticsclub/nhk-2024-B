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

 private:
  ikarashiCAN_mk2 can2;
  common::RohmMDBus rohm_md;
  common::CanServoBus can_servo;

 public:
  common::CanServo &servo_0;
  common::CanServo &servo_1;
  common::CanServo &servo_2;
  common::CanServo &servo_3;
  common::Rohm1chMD move_l;
  common::Rohm1chMD move_r;
  common::Rohm1chMD deploy;

  Actuators(Config config)
      : can1(config.can_1_rd, config.can_1_td, 0, (int)1E6),
        can2(config.can_2_rd, config.can_2_td, 0, (int)1E6),
        can_servo(can1, 1),
        rohm_md(),
        servo_0(*can_servo.NewNode(0)),
        servo_1(*can_servo.NewNode(1)),
        servo_2(*can_servo.NewNode(2)),
        servo_3(*can_servo.NewNode(3)),
        move_l(can1, 2),
        move_r(can1, 3),
        deploy(can1, 4) {}

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

    status = move_l.Send();
    if (status != 1) {
      errors |= 2;
    }
    status = move_r.Send();
    if (status != 1) {
      errors |= 4;
    }
    status = deploy.Send();
    if (status != 1) {
      errors |= 8;
    }

    return -errors;
  }

  void Tick() {
    can1.reset();
    can2.reset();
  }
};
}  // namespace nhk2024b::robot2