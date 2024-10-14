#pragma once

#pragma once

#include <robotics/node/node.hpp>
#include <robotics/types/joystick_2d.hpp>
#include <robotics/filter/angled_motor.hpp>
#include <robotics/filter/muxer.hpp>
#include <logger/logger.hpp>

namespace nhk2024b {
template <typename T>
using Node = robotics::Node<T>;
using JoyStick2D = robotics::JoyStick2D;

template <typename T>
using Muxer = robotics::filter::Muxer<T>;
}  // namespace nhk2024b

namespace nhk2024b::robot3 {

robotics::logger::Logger logger{"Robot3", "robot3"};

class Robot {
 public:
  Node<JoyStick2D> ctrl_stick_move;
  Node<bool> ctrl_button_arm_up;
  Node<bool> ctrl_button_arm_down;
  Node<bool> ctrl_button_arm_open;
  Node<bool> ctrl_button_arm_close;

  Node<float> out_move_l;
  Node<float> out_move_r;
  Node<float> out_arm_elevation;
  Node<float> out_arm_expansion;

 private:

  void UpdateMove() {
    auto stick = ctrl_stick_move.GetValue();

    auto left = (stick[0] + stick[1]) / 1.41;
    auto right = (stick[0] - stick[1]) / 1.41;

    if (abs(left) > 1 || abs(right) > 1) {
      auto max = std::max(abs(left), abs(right));
      left /= max;
      right /= max;
    }

    out_move_l.SetValue(left * 0.95);
    out_move_r.SetValue(right * 0.95);
  }

 public:
  Robot() {
  }

  void LinkController() {
    ctrl_stick_move.SetChangeCallback(
        [this](robotics::types::JoyStick2D) { UpdateMove(); });

    // ボタンの設定
    ctrl_button_arm_up.SetChangeCallback(
        [this](bool btn) { out_arm_elevation.SetValue(btn ? 1 : 0); });
    ctrl_button_arm_down.SetChangeCallback(
        [this](bool btn) { out_arm_elevation.SetValue(btn ? -1 : 0); });
    ctrl_button_arm_open.SetChangeCallback(
        [this](bool btn) { out_arm_expansion.SetValue(btn ? 1 : 0); });
    ctrl_button_arm_close.SetChangeCallback(
        [this](bool btn) { out_arm_expansion.SetValue(btn ? -1 : 0); });
  }
};
}  // namespace nhk2024b::robot3