#pragma once

#include "./fep_ps4_con.hpp"

#include <robotics/logger/logger.hpp>
#include <robotics/platform/thread.hpp>

namespace nhk2024b::test {
int test_ps4_fep() {
  robotics::logger::Logger logger{"ds4.fep.test.nhk2024b", "   Test  "};
  nhk2024b::ps4_con::PS4Con ctrl{PC_6, PC_7, 115200};

  ctrl.Init();

  ctrl.button_square.SetChangeCallback(
      [&logger](bool x) { logger.Info("button_square => %d", x); });
  ctrl.button_cross.SetChangeCallback(
      [&logger](bool x) { logger.Info("button_cross => %d", x); });
  ctrl.button_circle.SetChangeCallback(
      [&logger](bool x) { logger.Info("button_circle => %d", x); });
  ctrl.button_triangle.SetChangeCallback(
      [&logger](bool x) { logger.Info("button_triangle => %d", x); });
  ctrl.button_share.SetChangeCallback(
      [&logger](bool x) { logger.Info("button_share => %d", x); });
  ctrl.button_options.SetChangeCallback(
      [&logger](bool x) { logger.Info("button_options => %d", x); });
  ctrl.button_ps.SetChangeCallback(
      [&logger](bool x) { logger.Info("button_ps => %d", x); });
  ctrl.button_touchPad.SetChangeCallback(
      [&logger](bool x) { logger.Info("button_touchPad => %d", x); });

  ctrl.button_l1.SetChangeCallback(
      [&logger](bool a) { logger.Info("button_l1 => %f", a); });
  ctrl.trigger_l.SetChangeCallback(
      [&logger](float a) { logger.Info("trigger_l => %f", a); });
  ctrl.button_l3.SetChangeCallback(
      [&logger](bool a) { logger.Info("button_l3 => %f", a); });
  ctrl.button_r1.SetChangeCallback(
      [&logger](bool a) { logger.Info("button_r1 => %f", a); });
  ctrl.trigger_r.SetChangeCallback(
      [&logger](float a) { logger.Info("trigger_r => %f", a); });
  ctrl.button_r3.SetChangeCallback(
      [&logger](bool a) { logger.Info("button_r3 => %f", a); });

  ctrl.stick_left.SetChangeCallback([&logger](robotics::types::JoyStick2D stick) {
    logger.Info("left ==> (%lf, %lf)", stick[0], stick[1]);
  });
  ctrl.stick_right.SetChangeCallback([&logger](robotics::types::JoyStick2D stick) {
    logger.Info("right ==> (%lf, %lf)", stick[0], stick[1]);
  });

  while (1) {
    ctrl.Update();
    robotics::system::SleepFor(50ms);
  }
}
}  // namespace nhk2024b::test