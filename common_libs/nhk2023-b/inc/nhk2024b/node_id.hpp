#pragma once

namespace nhk2024b::node_id {

constexpr const int kController = 0x0001;
constexpr const int kRobot1 = 0xb732;
constexpr const int kRobot2 = 0xb732;

int GetPipe1Remote(int self_node_id) {
  return self_node_id == kController ? kRobot1 : kController;
}
int GetPipe2Remote(int self_node_id) {
  return self_node_id == kController ? kRobot2 : kController;
}

}