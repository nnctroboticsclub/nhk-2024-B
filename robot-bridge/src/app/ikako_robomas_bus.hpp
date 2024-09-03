#pragma once

#include <ikarashiCAN_mk2.h>
#include <mbed-robotics/ikakorobomas_node.hpp>

namespace nhk2024b::common {
class IkakoRobomasBus {
  IkakoRobomasSender sender;
  std::vector<IkakoRobomasNode *> nodes;

 public:
  IkakoRobomasBus(ikarashiCAN_mk2 &can) : sender(&can) {}

  int Write() { return sender.write(); }

  void Read() { sender.read(); }

  IkakoRobomasNode *NewNode(int index) {
    auto node = new IkakoRobomasNode(index);
    sender.set_motors(node->GetIkakoM3508().get_motor());
    nodes.push_back(node);
    return node;
  }

  // dt = 0.001 (1ms)
  void Update() {
    for (auto node : nodes) {
      node->Update();
    }
  }

  void Tick() { sender.read(); }
};
}  // namespace nhk2024b::common