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

  void RegisterTo(
      robotics::network::ssp::ValueStoreService<uint16_t> *value_store,
      uint16_t remote) {
    value_store->AddController(0x2400'0100, remote, move);
    value_store->AddController(0x2400'0101, remote, emc);
    value_store->AddController(0x2400'0102, remote, buttons);
    value_store->AddController(0x2400'0103, remote, rotation_ccw);
    value_store->AddController(0x2400'0104, remote, rotation_cw);
  }
};
}  // namespace nhk2024b::robot1