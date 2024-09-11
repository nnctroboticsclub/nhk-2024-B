#include <nhk2024b/ps4_con.hpp>

namespace robotics::node {
template <>
std::array<uint8_t, 4> NodeEncoder<nhk2024b::ps4_con::DPad>::Encode(
    nhk2024b::ps4_con::DPad value) {
  std::array<uint8_t, 4> data;
  data[0] = value;
  data[1] = 0;
  data[2] = 0;
  data[3] = 0;

  return data;
}

template <>
nhk2024b::ps4_con::DPad NodeEncoder<nhk2024b::ps4_con::DPad>::Decode(
    std::array<uint8_t, 4> data) {
  return static_cast<nhk2024b::ps4_con::DPad>(data[0]);
}

template <>
std::array<uint8_t, 4> NodeEncoder<nhk2024b::ps4_con::Buttons>::Encode(
    nhk2024b::ps4_con::Buttons value) {
  std::array<uint8_t, 4> data;
  data[0] =
      value.square << 3 | value.cross << 2 | value.circle << 1 | value.triangle;
  data[1] =
      value.share << 3 | value.options << 2 | value.ps << 1 | value.touchPad;
  data[2] = value.l1 << 3 | value.r1 << 2 | value.l3 << 1 | value.r3;
  data[3] = 0;

  return data;
}

template <>
nhk2024b::ps4_con::Buttons NodeEncoder<nhk2024b::ps4_con::Buttons>::Decode(
    std::array<uint8_t, 4> data) {
  nhk2024b::ps4_con::Buttons buttons;
  buttons.square = data[0] & 0x08;
  buttons.cross = data[0] & 0x04;
  buttons.circle = data[0] & 0x02;
  buttons.triangle = data[0] & 0x01;

  buttons.share = data[1] & 0x08;
  buttons.options = data[1] & 0x04;
  buttons.ps = data[1] & 0x02;
  buttons.touchPad = data[1] & 0x01;

  buttons.l1 = data[2] & 0x08;
  buttons.r1 = data[2] & 0x04;
  buttons.l3 = data[2] & 0x02;
  buttons.r3 = data[2] & 0x01;

  return buttons;
}
}  // namespace robotics::node