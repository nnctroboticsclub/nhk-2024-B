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

  Node<float> out_move;
  Node<float> out_arm_elevation;
  Node<float> out_arm_expansion;

  Node<bool> emc_state;

 private:
  Node<float> value_move;
  Node<float> value_arm_elevation;
  Node<float> value_arm_expansion;

  Node<float> zero;

  Muxer<float> move;
  Muxer<float> arm_elevation;
  Muxer<float> arm_expansion;

  void UpdateMove() {
    float forward_back = ctrl_stick_move.GetValue()[1];

    value_move.SetValue(forward_back);
  }

 public:
  Robot() {  //
    zero.SetValue(0);

    move.AddInput(zero);
    move.AddInput(value_move);
    move.output_ >> out_move;

    arm_elevation.AddInput(zero);
    arm_elevation.AddInput(value_arm_elevation);
    arm_elevation.output_ >> out_arm_elevation;
    // value_arm_elevation >> out_arm_elevation;

    arm_expansion.AddInput(zero);
    arm_expansion.AddInput(value_arm_expansion);
    arm_expansion.output_ >> out_arm_expansion;

    emc_state.SetChangeCallback([this](bool emc) {
      auto i = emc ? 1 : 0;
      logger.Info("Select i = %d", i);
      move.Select(i);
      arm_elevation.Select(i);
      arm_expansion.Select(i);
    });

    emc_state.SetValue(0);
  }

  void LinkController() {
    ctrl_stick_move.SetChangeCallback(
        [this](robotics::types::JoyStick2D) { UpdateMove(); });

    // ボタンの設定
    ctrl_button_arm_up.SetChangeCallback(
        [this](bool btn) { value_arm_elevation.SetValue(btn ? 1 : 0); });
    ctrl_button_arm_down.SetChangeCallback(
        [this](bool btn) { value_arm_elevation.SetValue(btn ? -1 : 0); });
    ctrl_button_arm_open.SetChangeCallback(
        [this](bool btn) { value_arm_expansion.SetValue(btn ? 1 : 0); });
    ctrl_button_arm_close.SetChangeCallback(
        [this](bool btn) { value_arm_expansion.SetValue(btn ? -1 : 0); });
  }
};
}  // namespace nhk2024b::robot3