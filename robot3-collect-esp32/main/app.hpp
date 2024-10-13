#pragma once

#include <bd621x/BD621x.hpp>
#include <robotics/driver/dout.hpp>
#include <robotics/driver/pwm.hpp>

#include <driver/gpio.h>

namespace nhk2024b::robot3 {
class Actuators {
 public:
  struct Config {
    gpio_num_t move_motor_fin;
    gpio_num_t move_motor_rin;
    gpio_num_t arm_elevation_motor_fin;
    gpio_num_t arm_elevation_motor_rin;
    gpio_num_t arm_expansion_motor_fin;
    gpio_num_t arm_expansion_motor_rin;

    ledc_channel_t ledc_channel;
    ledc_timer_t ledc_timer;
  };

  robotics::node::BD621x move_motor;
  robotics::node::BD621x arm_elevation_motor;
  robotics::node::BD621x arm_expansion_motor;

  Actuators(const Config& config)
      : move_motor{std::make_shared<robotics::driver::PWM>(
                       config.move_motor_fin, LEDC_CHANNEL_0,
                       config.ledc_timer),
                   std::make_shared<robotics::driver::PWM>(
                       config.move_motor_rin, LEDC_CHANNEL_0,
                       config.ledc_timer)},
        arm_elevation_motor{std::make_shared<robotics::driver::PWM>(
                                config.arm_elevation_motor_fin, LEDC_CHANNEL_0,
                                config.ledc_timer),
                            std::make_shared<robotics::driver::PWM>(
                                config.arm_elevation_motor_rin, LEDC_CHANNEL_0,
                                config.ledc_timer)},
        arm_expansion_motor{std::make_shared<robotics::driver::PWM>(
                                config.arm_expansion_motor_fin, LEDC_CHANNEL_0,
                                config.ledc_timer),
                            std::make_shared<robotics::driver::PWM>(
                                config.arm_expansion_motor_rin, LEDC_CHANNEL_0,
                                config.ledc_timer)} {
    move_motor.factor.SetValue(0.53);
    arm_elevation_motor.factor.SetValue(0.53);
    arm_expansion_motor.factor.SetValue(0.53);
  }
};
}  // namespace nhk2024b::robot3