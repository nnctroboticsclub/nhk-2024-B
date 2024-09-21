#pragma once

#include <robotics/logger/logger.hpp>
#include <robotics/node/node.hpp>
#include <robotics/network/ssp/ssp.hpp>
#include <robotics/network/ssp/value_store.hpp>
#include "../ps4_con.hpp"
#include "../types.hpp"

namespace nhk2024b::robot1 {
class Controller {
 public:
  Node<JoyStick2D> move;
  Node<ps4_con::DPad> buttons;
  Node<bool> emc;
  Node<float> rotation_cw;
  Node<float> rotation_ccw;

  std::array<uint8_t, 5> Pack() {
    std::array<uint8_t, 5> data{};
    data[0] = (move.GetValue()[0] + 1) * 255 / 2;
    data[1] = (move.GetValue()[1] + 1) * 255 / 2;
    data[2] = (rotation_cw.GetValue() + 1) * 255 / 2;
    data[3] = (rotation_ccw.GetValue() + 1) * 255 / 2;
    data[4] = ((buttons.GetValue() & 0x0f) << 1) | (emc.GetValue() ? 1 : 0);

    return data;
  }

  void Unpack(std::array<uint8_t, 5> const data) {
    auto stick_x = data[0] / 255.0f - 0.5f;
    auto stick_y = data[1] / 255.0f - 0.5f;
    auto stick = robotics::types::JoyStick2D(stick_x, stick_y);
    move.SetValue(stick);

    auto rotation_cw = data[2] / 255.0f - 0.5f;
  rotation_cw.SetValue(rotation_cw);

    auto rotation_ccw = data[3] / 255.0f - 0.5f;
    rotation_ccw.SetValue(rotation_ccw);

    auto button_dpad = data[5] >> 1;
    buttons.SetValue(reinterrupt_cast<ps4_con::DPad>(button_dpad));

    auto button_emc = data[5] & 1;
    emc.SetValue(button_emc);
  }
};
}  // namespace nhk2024b::robot1