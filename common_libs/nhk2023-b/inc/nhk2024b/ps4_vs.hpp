#pragma once

#include <robotics/logger/logger.hpp>
#include <robotics/node/node.hpp>
#include "ps4_con.hpp"
#include "types.hpp"

namespace vs_ps4 {
robotics::logger::Logger logger{"vs_ps4", "   PS4   "};

robotics::Node<robotics::types::JoyStick2D> stick_left;
robotics::Node<robotics::types::JoyStick2D> stick_right;
robotics::Node<nhk2024b::ps4_con::DPad> dpad;
robotics::Node<bool> button_square;
robotics::Node<bool> button_cross;
robotics::Node<bool> button_circle;
robotics::Node<bool> button_triangle;
robotics::Node<bool> button_share;
robotics::Node<bool> button_options;
robotics::Node<bool> button_ps;
robotics::Node<bool> button_touchPad;
robotics::Node<bool> button_l1;
robotics::Node<bool> button_r1;
robotics::Node<bool> button_l3;
robotics::Node<bool> button_r3;
robotics::Node<float> trigger_l;
robotics::Node<float> trigger_r;
robotics::Node<float> battery_level;

namespace state {
float stick_left_x = 0;
float stick_left_y = 0;
float stick_right_x = 0;
float stick_right_y = 0;
nhk2024b::ps4_con::DPad dpad_value = nhk2024b::ps4_con::DPad::kNone;
bool button_square_value = false;
bool button_cross_value = false;
bool button_circle_value = false;
bool button_triangle_value = false;
bool button_share_value = false;
bool button_options_value = false;
bool button_ps_value = false;
bool button_touchPad_value = false;
bool button_l1_value = false;
bool button_r1_value = false;
bool button_l3_value = false;
bool button_r3_value = false;
float trigger_l_value = 0;
float trigger_r_value = 0;
float battery_level_value = 0;

void Update() {
  stick_left.SetValue(robotics::types::JoyStick2D{stick_left_x, stick_left_y});
  stick_right.SetValue(
      robotics::types::JoyStick2D{stick_right_x, stick_right_y});
  dpad.SetValue(dpad_value);

  button_square.SetValue(button_square_value);
  button_cross.SetValue(button_cross_value);
  button_circle.SetValue(button_circle_value);
  button_triangle.SetValue(button_triangle_value);
  button_share.SetValue(button_share_value);
  button_options.SetValue(button_options_value);
  button_ps.SetValue(button_ps_value);
  button_touchPad.SetValue(button_touchPad_value);
  button_l1.SetValue(button_l1_value);
  button_r1.SetValue(button_r1_value);
  button_l3.SetValue(button_l3_value);
  button_r3.SetValue(button_r3_value);
  trigger_l.SetValue(trigger_l_value);
  trigger_r.SetValue(trigger_r_value);
  battery_level.SetValue(battery_level_value);
}
}  // namespace state

void RegisterWatcher() {
  stick_left.SetChangeCallback([](robotics::types::JoyStick2D v) {
    logger.Info("[Local] stick_left: %lf %lf", v[0], v[1]);
  });
  stick_right.SetChangeCallback([](robotics::types::JoyStick2D v) {
    logger.Info("[Local] stick_right: %lf %lf", v[0], v[1]);
  });
  dpad.SetChangeCallback(
      [](nhk2024b::ps4_con::DPad v) { logger.Info("[Local] dpad: %d", v); });
  button_square.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_square: %d", v); });
  button_cross.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_cross: %d", v); });
  button_circle.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_circle: %d", v); });
  button_triangle.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_triangle: %d", v); });
  button_share.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_share: %d", v); });
  button_options.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_options: %d", v); });
  button_ps.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_ps: %d", v); });
  button_touchPad.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_touchPad: %d", v); });
  button_l1.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_l1: %d", v); });
  button_r1.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_r1: %d", v); });
  button_l3.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_l3: %d", v); });
  button_r3.SetChangeCallback(
      [](bool v) { logger.Info("[Local] button_r3: %d", v); });
  trigger_l.SetChangeCallback(
      [](float v) { logger.Info("[Local] trigger_l: %lf", v); });
  trigger_r.SetChangeCallback(
      [](float v) { logger.Info("[Local] trigger_r: %lf", v); });
  battery_level.SetChangeCallback(
      [](float v) { logger.Info("[Local] battery_level: %lf", v); });
}

};  // namespace vs_ps4