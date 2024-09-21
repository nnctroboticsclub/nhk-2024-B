#pragma once

#include <robotics/node/BD621x.hpp>
#include <robotics/platform/dout.mbed.hpp>
#include <robotics/platform/pwm.mbed.hpp>

namespace nhk2024b::robot3 {
class Actuators {
 public:
  struct Config {
    PinName move_motor_fin;
    PinName move_motor_rin;
    PinName arm_elevation_motor_fin;
    PinName arm_elevation_motor_rin;
    PinName arm_expansion_motor_fin;
    PinName arm_expansion_motor_rin;
  };

  robotics::node::BD621x move_motor;
  robotics::node::BD621x arm_elevation_motor;
  robotics::node::BD621x arm_expansion_motor;

  Actuators(const Config& config)
      : move_motor{std::make_shared<robotics::driver::PWM>(
                       config.move_motor_fin),
                   std::make_shared<robotics::driver::PWM>(
                       config.move_motor_rin)},
        arm_elevation_motor{std::make_shared<robotics::driver::PWM>(
                                config.arm_elevation_motor_fin),
                            std::make_shared<robotics::driver::PWM>(
                                config.arm_elevation_motor_rin)},
        arm_expansion_motor{std::make_shared<robotics::driver::PWM>(
                                config.arm_expansion_motor_fin),
                            std::make_shared<robotics::driver::PWM>(
                                config.arm_expansion_motor_rin)} {
    move_motor.factor.SetValue(0.53);
    arm_elevation_motor.factor.SetValue(0.53);
    arm_expansion_motor.factor.SetValue(0.53);
  }
};
}  // namespace nhk2024b::robot3