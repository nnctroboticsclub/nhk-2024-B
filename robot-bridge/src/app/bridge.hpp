#pragma once

#include <nhk2024b/types.hpp>
#include "value_test.hpp"

namespace nhk2024b::robot2 {
class Robot {
  bool is_unlocked = false;
  float unlock_duty = 0.5;

 public:
  Node<JoyStick2D> ctrl_move;
  Node<bool> ctrl_deploy;
  Node<bool> ctrl_bridge_toggle;

  Node<float> out_bridge_unlock_duty;
  Node<float> out_deploy;
  Node<float> out_move_l;
  Node<float> out_move_r;

  Node<bool> ctrl_test_increment;
  Node<bool> ctrl_test_decrement;
  Node<float> out_unlock_duty;

  void LinkController() {
    ctrl_deploy.SetChangeCallback(
        [this](bool btn) { out_deploy.SetValue(btn ? 0.3 : 0); });
    ctrl_move.SetChangeCallback([this](robotics::types::JoyStick2D stick) {
      auto left = (stick[0] + stick[1]) / 1.41;
      auto right = (stick[0] - stick[1]) / 1.41;

      if (abs(left) > 1 || abs(right) > 1) {
        auto max = std::max(abs(left), abs(right));
        left /= max;
        right /= max;
      }

      out_move_l.SetValue(left * 0.95);
      out_move_r.SetValue(right * 0.95);
    });

    ctrl_bridge_toggle.SetChangeCallback([this](bool value) {
      is_unlocked = is_unlocked ^ value;
      out_bridge_unlock_duty.SetValue(is_unlocked ? 0.60 : -0.05);
    });

    ctrl_test_increment.SetChangeCallback([this](bool btn) {
      if (btn) unlock_duty += 1 / 20;
      out_unlock_duty.SetValue(unlock_duty);
    });

    ctrl_test_decrement.SetChangeCallback([this](bool btn) {
      if (btn) unlock_duty -= 1 / 20;
      out_unlock_duty.SetValue(unlock_duty);
    });
  }
};
}  // namespace nhk2024b::robot2