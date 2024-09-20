#pragma once

#include <nhk2024b/types.hpp>

namespace nhk2024b::robot3 {
class Robot  // ノードにデータを送るよ
{
 public:
  Node<JoyStick2D> ctrl_stick_rotate;
  Node<JoyStick2D> ctrl_stick_forward_back;
  Node<bool> ctrl_button_arm_up;
  Node<bool> ctrl_button_arm_down;
  Node<bool> ctrl_button_arm_open;
  Node<bool> ctrl_button_arm_close;

  Node<float> out_move_left;
  Node<float> out_move_right;
  Node<float> out_arm_elevation;
  Node<float> out_arm_extension;

  Node<bool> emc_state;

 private:
  float delta_arm_elevation_ = 0;
  float delta_arm_extension_ = 0;

  Node<float> value_move_left;
  Node<float> value_move_right;
  Node<float> value_arm_elevation;
  Node<float> value_arm_extension;

  Node<float> zero;

  Muxer<float> move_left;
  Muxer<float> move_right;
  Muxer<float> arm_elevation;
  Muxer<float> arm_extension;

  void UpdateMove() {
    float forward_back = ctrl_stick_forward_back.GetValue()[1];
    float rotate = ctrl_stick_rotate.GetValue()[0];

    float left = (forward_back + rotate);
    float right = (forward_back - rotate);

    if (left > 1) left = 1;
    if (right > 1) right = 1;

    value_move_left.SetValue(left * 1);
    value_move_right.SetValue(-right * 1);
  }

 public:
  Robot() {  //
    zero.SetValue(0);

    move_left.AddInput(zero);
    move_right.AddInput(zero);
    arm_elevation.AddInput(zero);
    arm_extension.AddInput(zero);

    move_left.AddInput(value_move_left);
    move_right.AddInput(value_move_right);
    arm_elevation.AddInput(value_arm_elevation);
    arm_extension.AddInput(value_arm_extension);

    emc_state.SetChangeCallback([this](bool emc) {
      auto i = emc ? 1 : 0;
      logger.Info("Select i = %d", i);
      move_left.Select(i);
      move_right.Select(i);
      arm_elevation.Select(i);
      arm_extension.Select(i);
    });

    move_left.output_ >> out_move_left;
    move_right.output_ >> out_move_right;
    arm_elevation.output_ >> out_arm_elevation;
    arm_extension.output_ >> out_arm_extension;

    emc_state.SetValue(0);
  }

  void LinkController() {
    ctrl_stick_rotate.SetChangeCallback(
        [this](robotics::types::JoyStick2D) { UpdateMove(); });

    ctrl_stick_forward_back.SetChangeCallback(
        [this](robotics::types::JoyStick2D) { UpdateMove(); });

    // ボタンの設定
    ctrl_button_arm_up.SetChangeCallback(
        [this](bool btn) { value_arm_elevation.SetValue(btn ? 1 : 0); });
    ctrl_button_arm_down.SetChangeCallback(
        [this](bool btn) { value_arm_elevation.SetValue(btn ? -1 : 0); });
    ctrl_button_arm_open.SetChangeCallback(
        [this](bool btn) { value_arm_extension.SetValue(btn ? 1 : 0); });
    ctrl_button_arm_close.SetChangeCallback(
        [this](bool btn) { value_arm_extension.SetValue(btn ? -1 : 0); });
  }
};
}  // namespace nhk2024b::robot3