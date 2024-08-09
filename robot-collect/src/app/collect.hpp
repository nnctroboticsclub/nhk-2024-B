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

 private:
  float delta_arm_elevation_ = 0;
  float delta_arm_extension_ = 0;

  void UpdateMove() {
    float forward_back = ctrl_stick_forward_back.GetValue()[1];
    float rotate = ctrl_stick_rotate.GetValue()[0];

    float left = forward_back + rotate;
    float right = forward_back - rotate;

    out_move_left.SetValue(left * 0.4);
    out_move_right.SetValue(right * 0.4);
  }

 public:
  void LinkController() {
    ctrl_stick_rotate.SetChangeCallback(
        [this](robotics::types::JoyStick2D) { UpdateMove(); });

    ctrl_stick_forward_back.SetChangeCallback(
        [this](robotics::types::JoyStick2D) { UpdateMove(); });

    // ボタンの設定
    ctrl_button_arm_up.SetChangeCallback(
        [this](bool btn) { out_arm_elevation.SetValue(btn ? 0.4 : 0); });
    ctrl_button_arm_down.SetChangeCallback(
        [this](bool btn) { out_arm_elevation.SetValue(btn ? -0.4 : 0); });
    ctrl_button_arm_open.SetChangeCallback(
        [this](bool btn) { out_arm_extension.SetValue(btn ? 0.4 : 0); });
    ctrl_button_arm_close.SetChangeCallback(
        [this](bool btn) { out_arm_extension.SetValue(btn ? -0.4 : 0); });
  }
};
}  // namespace nhk2024b