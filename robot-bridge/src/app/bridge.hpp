#pragma once

#include <nhk2024b/types.hpp>

namespace nhk2024b::robot2 {
class Robot {
  bool is_unlocked = false;

 public:
  Node<JoyStick2D> ctrl_move;
  Node<bool> ctrl_deploy;
  Node<bool> ctrl_bridge_toggle;

  Node<float> out_unlock_duty;
  Node<float> out_deploy;
  Node<float> out_move_l;
  Node<float> out_move_r;

  void LinkController() {
    ctrl_deploy.SetChangeCallback(
        [this](bool btn) { out_deploy.SetValue(btn ? 0.3 : 0); });
    ctrl_move.SetChangeCallback([this](robotics::types::JoyStick2D stick) {
      auto left = (stick[0] - stick[1]) / 1.4142135623730951;
      auto right = (stick[0] + stick[1]) / 1.4142135623730951;

      out_move_l.SetValue(left);
      out_move_r.SetValue(right);
    });

    ctrl_bridge_toggle.SetChangeCallback([this](bool value) {
      is_unlocked = is_unlocked ^ value;
      out_unlock_duty.SetValue(is_unlocked ? 0.60 : -0.05);
    });
  }
};
}  // namespace nhk2024b::robot2