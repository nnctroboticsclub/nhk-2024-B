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