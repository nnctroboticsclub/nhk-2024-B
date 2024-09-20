#pragma once

#include <robotics/node/BD621x.hpp>
#include <robotics/platform/dout.mbed.hpp>
#include <robotics/platform/pwm.mbed.hpp>

namespace nhk2024b::robot3 {
class Actuators {
 public:
  struct Config {
    PinName move_motor_l_fin;
    PinName move_motor_l_rin;
    PinName move_motor_r_fin;
    PinName move_motor_r_rin;
    PinName arm_elevation_motor_fin;
    PinName arm_elevation_motor_rin;
    PinName arm_extension_motor_fin;
    PinName arm_extension_motor_rin;
  };

  robotics::node::BD621x move_motor_l;
  robotics::node::BD621x move_motor_r;
  robotics::node::BD621x arm_elevation_motor;
  robotics::node::BD621x arm_extension_motor;

  Actuators(const Config& config)
      : move_motor_l{std::make_shared<robotics::driver::PWM>(
                         config.move_motor_l_fin),
                     std::make_shared<robotics::driver::PWM>(
                         config.move_motor_l_rin)},
        move_motor_r{
            std::make_shared<robotics::driver::PWM>(config.move_motor_r_fin),
            std::make_shared<robotics::driver::PWM>(config.move_motor_r_rin)},
        arm_elevation_motor{std::make_shared<robotics::driver::PWM>(
                                config.arm_elevation_motor_fin),
                            std::make_shared<robotics::driver::PWM>(
                                config.arm_elevation_motor_rin)},
        arm_extension_motor{std::make_shared<robotics::driver::PWM>(
                                config.arm_extension_motor_fin),
                            std::make_shared<robotics::driver::PWM>(
                                config.arm_extension_motor_rin)} {}
};
}  // namespace nhk2024b::robot3