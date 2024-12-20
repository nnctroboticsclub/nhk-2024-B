#pragma once

#include <logger/logger.hpp>
#include <robotics/node/node.hpp>
#include <ssp/ssp.hpp>
#include <ssp/value_store.hpp>
#include "../ps4_con.hpp"
#include "../types.hpp"

namespace nhk2024b::robot2 {
class Controller {
 public:
  Node<JoyStick2D> move;
  Node<bool> emc;
  Node<bool> button_deploy;
  Node<bool> button_bridge_toggle;
  Node<bool> button_unassigned0;
  Node<bool> button_unassigned1;

  Node<bool> test_increase;
  Node<bool> test_decrease;

  void RegisterTo(
      robotics::network::ssp::ValueStoreService<uint16_t, bool> *value_store,
      uint16_t remote) {
    value_store->AddController(0x2400'0200, remote, move);
    value_store->AddController(0x2400'0201, remote, emc);
    value_store->AddController(0x2400'0202, remote, button_deploy);
    value_store->AddController(0x2400'0203, remote, button_bridge_toggle);
    value_store->AddController(0x2400'0204, remote, button_unassigned0);
    value_store->AddController(0x2400'0205, remote, button_unassigned1);
    value_store->AddController(0x2400'0206, remote, test_increase);
    value_store->AddController(0x2400'0207, remote, test_decrease);
  }
};
}  // namespace nhk2024b::robot2