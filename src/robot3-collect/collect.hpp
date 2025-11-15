#pragma once

#include <nhk2024b/types.hpp>

namespace nhk2024b::robot3 {
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

    arm_expansion.AddInput(zero);
    arm_expansion.AddInput(value_arm_expansion);
    arm_expansion.output_ >> out_arm_expansion;

    emc_state >> [this](bool emc) {
      auto i = emc ? 1 : 0;
      logger.Info("Select i = %d", i);
      move.Select(i);
      arm_elevation.Select(i);
      arm_expansion.Select(i);
    };

    emc_state.SetValue(false);
  }

  void LinkController() {
    ctrl_stick_move >> ([this](robotics::types::JoyStick2D) { UpdateMove(); });

    // ボタンの設定
    ctrl_button_arm_up >> [this](bool btn) {
      value_arm_elevation.SetValue(btn ? 1 : 0);
    };
    ctrl_button_arm_down >> [this](bool btn) {
      value_arm_elevation.SetValue(btn ? -1 : 0);
    };
    ctrl_button_arm_open >> [this](bool btn) {
      value_arm_expansion.SetValue(btn ? 1 : 0);
    };
    ctrl_button_arm_close >> [this](bool btn) {
      value_arm_expansion.SetValue(btn ? -1 : 0);
    };
  }
};
}  // namespace nhk2024b::robot3