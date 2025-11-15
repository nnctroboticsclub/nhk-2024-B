#pragma once

#include <ikarashiCAN_mk2.h>
#include "rohm_md.hpp"

namespace nhk2024b::common {
class RohmMDBus {
  std::vector<Rohm1chMD *> nodes;

 public:
  RohmMDBus() {}

  int Send() {
    for (auto &&node : nodes) {
      if (node->Send() != 1) {
        return 0;
      }
    }
    return 1;
  }

  void Read() {
    for (auto &&node : nodes) {
      node->Read();
    }
  }

  Rohm1chMD *NewNode(ikarashiCAN_mk2 *can, int id) {
    auto p = new Rohm1chMD(*can, id);
    printf("Rohm 1ch MD placed at %p\n", p);

    nodes.emplace_back(p);
    return p;
  }
};
}  // namespace nhk2024b::common