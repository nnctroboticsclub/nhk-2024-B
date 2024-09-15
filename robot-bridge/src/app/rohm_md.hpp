#pragma once

#include <rohm_md.h>
#include <ikarashiCAN_mk2.h>

#include <robotics/node/node.hpp>

namespace nhk2024b::common {
class aRohm1chMD {
  ::RohmMD md;

 public:
  robotics::Node<float> in_velocity;
  robotics::Node<float> out_velocity;  // [rad/s]
  robotics::Node<float> out_current;   // [A]
  robotics::Node<float> out_radian;    // [rad]

  Rohm1chMD(ikarashiCAN_mk2 &can, int id) : md(&can, id) {
    in_velocity.SetChangeCallback([this](float v) { md.set(0, v); });
  }

  bool Send() { return md.send(); }

  bool Read() {
    auto r = md.read();
    out_velocity.SetValue(md.get_vel());
    out_current.SetValue(md.get_cur());
    out_radian.SetValue(md.get_rad());

    return r;
  }
};
}  // namespace nhk2024b::common