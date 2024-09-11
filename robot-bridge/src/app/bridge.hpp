#pragma once

#include <nhk2024b/types.hpp>

namespace nhk2024b::robot2 {
class Robot {
  template <typename T>
  using Node = robotics::Node<T>;

  float unlock_duty_ = -0.05;
  bool is_unlocked = false;
 public:
  Node<JoyStick2D> ctrl_move;
  Node<bool> ctrl_deploy;
  // Node<bool> ctrl_test_unlock_dec;
  // Node<bool> ctrl_test_unlock_inc;
  
  Node<bool> ctrl_bridge_toggle; // 0.60

  Node<float> out_unlock_duty;
  Node<float> out_deploy;
  Node<float> out_move_l;
  Node<float> out_move_r;

  void LinkController() {
    ctrl_deploy.SetChangeCallback([this](bool value) {
      if (value) {
        out_deploy.SetValue(0.2);
      } else {
        out_deploy.SetValue(0);
      }
    });
    ctrl_move.SetChangeCallback([this](robotics::types::JoyStick2D stick) {
      auto left = stick[1] + stick[0];
      auto right = stick[1] - stick[0];

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