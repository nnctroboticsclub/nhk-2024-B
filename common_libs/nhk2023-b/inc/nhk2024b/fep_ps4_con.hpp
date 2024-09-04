#pragma once

#include "types.hpp"
#include "ps4_con.hpp"

#include <PS4.h>
#include <robotics/node/node.hpp>
#include <robotics/types/joystick_2d.hpp>
#include <robotics/logger/logger.hpp>

namespace nhk2024b::ps4_con {

class PS4Con {
  static robotics::logger::Logger logger;

 public:
  PS4 ps4;

 public:
  robotics::Node<robotics::types::JoyStick2D> stick_left;
  robotics::Node<robotics::types::JoyStick2D> stick_right;
  robotics::Node<DPad> dpad;
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

 private:
  void SyncDPad() {
    bool up = ps4.getButton(UP);
    bool down = ps4.getButton(DOWN);
    bool left = ps4.getButton(LEFT);
    bool right = ps4.getButton(RIGHT);

    DPad value = DPad::kNone;

    if (up) {
      value = DPad(value | DPad::kUp);
    }
    if (down) {
      value = DPad(value | DPad::kDown);
    }
    if (left) {
      value = DPad(value | DPad::kLeft);
    }
    if (right) {
      value = DPad(value | DPad::kRight);
    }

    dpad.SetValue(value);
  }
  void SyncPS4ToNode() {
    if (!ps4.getStatus()) return;

    auto left = robotics::types::JoyStick2D();
    left[0] = ps4.getStick(0) / 128.0f;
    left[1] = ps4.getStick(1) / 128.0f;
    stick_left.SetValue(left);

    auto right = robotics::types::JoyStick2D();
    right[0] = ps4.getStick(2) / 128.0f;
    right[1] = ps4.getStick(3) / 128.0f;
    stick_right.SetValue(right);

    SyncDPad();

    button_square.SetValue(ps4.getButton(SQUARE));
    button_cross.SetValue(ps4.getButton(CROSS));
    button_circle.SetValue(ps4.getButton(CIRCLE));
    button_triangle.SetValue(ps4.getButton(TRIANGLE));

    button_share.SetValue(ps4.getButton(SHARE));
    button_options.SetValue(ps4.getButton(OPTIONS));
    button_ps.SetValue(ps4.getButton(PS));
    button_touchPad.SetValue(ps4.getButton(TOUCHPAD));

    button_l1.SetValue(ps4.getButton(L1));
    button_r1.SetValue(ps4.getButton(R1));
    button_l3.SetValue(ps4.getButton(L3));
    button_r3.SetValue(ps4.getButton(R3));

    trigger_l.SetValue(ps4.getTrigger(L2) * 1.0f / 0x7fff);
    trigger_r.SetValue(ps4.getTrigger(R2) * 1.0f / 0x7fff);
  }

 public:
  PS4Con(PinName tx, PinName rx, int baud = 115200) : ps4(tx, rx, baud) {}

  void Init() { this->ps4.StartReceive(); }

  void Update() { SyncPS4ToNode(); }
};

robotics::logger::Logger PS4Con::logger =
    robotics::logger::Logger("PS4Con", "ps4bcon");
}  // namespace nhk2024b::ps4_con