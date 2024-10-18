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

 private:
  common::RohmMDBus rohm_md;
  common::CanServoBus can_servo;
  common::IkakoRobomasBus robomas_bus;

 public:
  common::CanServo &servo_0;
  common::CanServo &servo_1;
  common::CanServo &servo_2;
  common::CanServo &servo_3;
  IkakoRobomasNode &move_l;
  IkakoRobomasNode &move_r;
  IkakoRobomasNode &dummy3;
  IkakoRobomasNode &dummy4;
  IkakoRobomasNode &dummy5;
  IkakoRobomasNode &dummy6;
  IkakoRobomasNode &dummy7;
  IkakoRobomasNode &dummy8;
  common::Rohm1chMD deploy;

  explicit Actuators(Config const &config)
      : can1(config.can_1_rd, config.can_1_td, 0, (int)1E6),
        can2(config.can_2_rd, config.can_2_td, 0, (int)1E6),
        can_servo(can1, 1),
        robomas_bus(can2),
        servo_0(*can_servo.NewNode(0)),
        servo_1(*can_servo.NewNode(1)),
        servo_2(*can_servo.NewNode(2)),
        servo_3(*can_servo.NewNode(3)),
        dummy3(*robomas_bus.NewNode(1, new IkakoM3508(1))),
        // these id is not correct. but it's okay for now.
        move_r(*robomas_bus.NewNode(2, new IkakoM3508(2))),
        move_l(*robomas_bus.NewNode(3, new IkakoM3508(3))),
        dummy4(*robomas_bus.NewNode(4, new IkakoM3508(4))),
        dummy5(*robomas_bus.NewNode(5, new IkakoM3508(5))),
        dummy6(*robomas_bus.NewNode(6, new IkakoM3508(6))),
        dummy7(*robomas_bus.NewNode(7, new IkakoM3508(7))),
        dummy8(*robomas_bus.NewNode(8, new IkakoM3508(8))),
        deploy(can1, 5) {}

  void Init() {
    can1.read_start();
    can2.read_start();

    move_l.velocity.SetValue(0);
    move_r.velocity.SetValue(0);
    dummy3.velocity.SetValue(0);
    dummy4.velocity.SetValue(0);
    dummy5.velocity.SetValue(0);
    dummy6.velocity.SetValue(0);
    dummy7.velocity.SetValue(0);
    dummy8.velocity.SetValue(0);
  }

  int Read() {
    rohm_md.Read();
    robomas_bus.Read();
    return 0;
  }

  int Send() {
    static int errors = 0;
    static int status = 0;

    status = (status + 1) % 2;

    switch (status) {
      case 0:
        errors = robomas_bus.Write() ? errors | 2 : errors & ~2;
        can2.set_this_id(0);
        break;
      case 1:
        errors = deploy.Send() ? errors | 4 : errors & ~4;
        errors = can_servo.Send() ? errors | 1 : errors & ~1;
        break;

      default:
        //
        break;
    }

    return -errors;
  }

  void Tick() {
    can1.reset();
    can2.reset();
    robomas_bus.Update();
  }
};
}  // namespace nhk2024b::robot2