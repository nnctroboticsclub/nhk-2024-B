#pragma once

#include <ikarashiCAN_mk2.h>
#include "rohm_md.hpp"

namespace nhk2024b::common {
class RohmMDBus {
  std::vector<Rohm1chMD *> nodes;
  ikarashiCAN_mk2 &can;

 public:
  RohmMDBus(ikarashiCAN_mk2 &can) : can(can) {}

  int Send() {
    for (auto &&node : nodes) {
      node->Send();
    }
  }

  void Read() {
    for (auto &&node : nodes) {
      node->Read();
    }
  }

  Rohm1chMD &NewNode(int id) {
    nodes.emplace_back(new Rohm1chMD(can, id));
    return *nodes.back();
  }
};
}  // namespace nhk2024b::common