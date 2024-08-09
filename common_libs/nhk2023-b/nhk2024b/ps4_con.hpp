#pragma once

#include <cstddef>

#include <robotics/node/node.hpp>

namespace nhk2024b::ps4_con {
enum DPad {
  kUp = 0x01,
  kDown = 0x02,
  kLeft = 0x04,
  kRight = 0x08,

  kUpRight = kUp | kRight,
  kDownRight = kDown | kRight,

  kUpLeft = kUp | kLeft,
  kDownLeft = kDown | kLeft,

  kNone = 0x00,
};

struct Buttons {
  uint8_t square : 1;
  uint8_t cross : 1;
  uint8_t circle : 1;
  uint8_t triangle : 1;

  uint8_t share : 1;
  uint8_t options : 1;
  uint8_t ps : 1;
  uint8_t touchPad : 1;

  uint8_t l1 : 1;
  uint8_t r1 : 1;
  uint8_t l3 : 1;
  uint8_t r3 : 1;
} __attribute__((packed));
}  // namespace nhk2024b::ps4_con

namespace robotics::node {
template <>
class NodeEncoder<nhk2024b::ps4_con::DPad> : public NodeEncoder<void> {
 public:
  NodeEncoder() : NodeEncoder<void>() {}
  void Update(nhk2024b::ps4_con::DPad value) {
    std::array<uint8_t, 4> data;
    data[0] = value;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;

    inspector.Update(data);
  }
};

template <>
class NodeEncoder<nhk2024b::ps4_con::Buttons> : public NodeEncoder<void> {
 public:
  NodeEncoder() : NodeEncoder<void>() {}
  void Update(nhk2024b::ps4_con::Buttons value) {
    std::array<uint8_t, 4> data;
    data[0] = value.square << 3 | value.cross << 2 | value.circle << 1 |
              value.triangle;
    data[1] =
        value.share << 3 | value.options << 2 | value.ps << 1 | value.touchPad;
    data[2] = value.l1 << 3 | value.r1 << 2 | value.l3 << 1 | value.r3;
    data[3] = 0;

    inspector.Update(data);
  }
};
}  // namespace robotics::node