#pragma once

#include <robotics/node/node.hpp>
#include <robotics/types/joystick_2d.hpp>
#include <robotics/filter/angled_motor.hpp>

namespace nhk2024b {
template <typename T>
using Node = robotics::Node<T>;
using JoyStick2D = robotics::JoyStick2D;
extern robotics::logger::Logger logger;
}  // namespace nhk2024b { "nhk2024b", "NHK2024-B" }
