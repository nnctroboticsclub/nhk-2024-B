#pragma once

#include <ikarashiCAN_mk2.h>

// #include "rohm_md_bus.hpp"

#ifdef R2_USE_ROBOMAS
#include "ikako_robomas_bus.hpp"
#endif

#ifdef R2_USE_SERVO
#include "can_servo.hpp"
#endif

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
#ifdef R2_USE_SERVO
  common::CanServoBus can_servo;
#endif

#ifdef R2_USE_ROBOMAS
  common::IkakoRobomasBus robomas_bus;
#endif

 public:
#ifdef R2_USE_SERVO
  common::CanServo &servo_0;
  common::CanServo &servo_1;
  common::CanServo &servo_2;
  common::CanServo &servo_3;
#endif

#ifdef R2_USE_ROBOMAS
  IkakoRobomasNode &dummy1;
  IkakoRobomasNode &move_r;
  IkakoRobomasNode &move_l;
  IkakoRobomasNode &dummy4;

  IkakoRobomasNode &deploy;
  IkakoRobomasNode &dummy6;
  IkakoRobomasNode &dummy7;
  IkakoRobomasNode &dummy8;
#endif

  int dummy_variable = 0;

  explicit Actuators(Config const &config)
      : can1(config.can_1_rd, config.can_1_td, 0, (int)1E6),
        can2(config.can_2_rd, config.can_2_td, 0, (int)1E6),

#ifdef R2_USE_SERVO
        can_servo(can1, 1),
#endif
#ifdef R2_USE_ROBOMAS
        robomas_bus(can2),
#endif

#ifdef R2_USE_SERVO
        servo_0(*can_servo.NewNode(3)),
        servo_1(*can_servo.NewNode(4)),
        servo_2(*can_servo.NewNode(5)),
        servo_3(*can_servo.NewNode(6)),
#endif

#ifdef R2_USE_ROBOMAS
        dummy1(*robomas_bus.NewNode(1, new IkakoM2006(1), 20.0f)),
        move_r(*robomas_bus.NewNode(2, new IkakoM3508(2), 20.0f)),
        move_l(*robomas_bus.NewNode(3, new IkakoM3508(3), 20.0f)),
        dummy4(*robomas_bus.NewNode(4, new IkakoM3508(4), 20.0f)),

        deploy(*robomas_bus.NewNode(5, new IkakoM2006(5), 10.0f)),
        dummy6(*robomas_bus.NewNode(6, new IkakoM3508(6), 20.0f)),
        dummy7(*robomas_bus.NewNode(7, new IkakoM3508(7), 20.0f)),
        dummy8(*robomas_bus.NewNode(8, new IkakoM3508(8), 20.0f))
#endif
        dummy_variable(0)
        {}

  void Init() {
    can1.read_start();
    can2.read_start();

#ifdef R2_USE_ROBOMAS
    dummy1.velocity.SetValue(0);
    move_l.velocity.SetValue(0);
    move_r.velocity.SetValue(0);
    dummy4.velocity.SetValue(0);
    deploy.velocity.SetValue(0);
    dummy6.velocity.SetValue(0);
    dummy7.velocity.SetValue(0);
    dummy8.velocity.SetValue(0);
#endif
  }

  int Read() {
#ifdef R2_USE_ROBOMAS
    return robomas_bus.Read();
#endif
    return 0;
  }

  int Send() {
    static int errors = 0;
    static int status = 0;

    status = (status + 1) % 2;

    switch (status) {
      case 0:
#ifdef R2_USE_ROBOMAS
        errors = robomas_bus.Write() ? errors | 2 : errors & ~2;
        can2.set_this_id(0);
#endif
        break;
      case 1:
#ifdef R2_USE_SERVO
        errors = can_servo.Send() ? errors | 1 : errors & ~1;
#endif
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
#ifdef R2_USE_ROBOMAS
    robomas_bus.Update();
#endif
#ifdef R2_USE_SERVO
    can_servo.Send();
#endif
  }
};
}  // namespace nhk2024b::robot2