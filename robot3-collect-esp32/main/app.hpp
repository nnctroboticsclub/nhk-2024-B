#pragma once

#include <bd621x/BD621x.hpp>
#include <bd621x/BD621xFull.hpp>
#include <robotics/driver/dout.hpp>
#include <robotics/driver/pwm.hpp>

#include <driver/gpio.h>

namespace nhk2024b::robot3 {
class Actuators {
 public:
  struct Config {
    gpio_num_t move_l_motor_fin;
    ledc_channel_t move_l_forward_channel;
    gpio_num_t move_l_motor_rin;
    ledc_channel_t move_l_reverse_channel;

    gpio_num_t move_r_motor_fin;
    ledc_channel_t move_r_forward_channel;
    gpio_num_t move_r_motor_rin;
    ledc_channel_t move_r_reverse_channel;

    gpio_num_t arm_elevation_motor_fin;
    gpio_num_t arm_elevation_motor_rin;

    gpio_num_t arm_expansion_motor_fin;
    gpio_num_t arm_expansion_motor_rin;

    ledc_timer_t ledc_timer;
  };

  robotics::node::BD621x move_l_motor;
  robotics::node::BD621x move_r_motor;
  robotics::node::BD621xFull arm_elevation_motor;
  robotics::node::BD621xFull arm_expansion_motor;

  Actuators(const Config& config)
      : move_l_motor{std::make_shared<robotics::driver::PWM>(
                         config.move_l_motor_fin, config.move_l_forward_channel,
                         config.ledc_timer),
                     std::make_shared<robotics::driver::PWM>(
                         config.move_l_motor_rin, config.move_l_reverse_channel,
                         config.ledc_timer)},
        move_r_motor{std::make_shared<robotics::driver::PWM>(
                         config.move_r_motor_fin, config.move_r_forward_channel,
                         config.ledc_timer),
                     std::make_shared<robotics::driver::PWM>(
                         config.move_r_motor_rin, config.move_r_reverse_channel,
                         config.ledc_timer)},
        arm_elevation_motor{std::make_shared<robotics::driver::Dout>(
                                config.arm_elevation_motor_fin),
                            std::make_shared<robotics::driver::Dout>(
                                config.arm_elevation_motor_rin)},
        arm_expansion_motor{std::make_shared<robotics::driver::Dout>(
                                config.arm_expansion_motor_fin),
                            std::make_shared<robotics::driver::Dout>(
                                config.arm_expansion_motor_rin)} {
    move_l_motor.factor.SetValue(0.53);
    move_r_motor.factor.SetValue(0.53);
    arm_elevation_motor.factor.SetValue(0.53);
    arm_expansion_motor.factor.SetValue(0.53);
  }
};
}  // namespace nhk2024b::robot3