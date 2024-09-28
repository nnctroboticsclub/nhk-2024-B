#pragma once

#include <robotics/node/node.hpp>
#include <robotics/types/joystick_2d.hpp>
#include <robotics/filter/angled_motor.hpp>
#include <robotics/filter/muxer.hpp>
#include <logger/logger.hpp>

namespace nhk2024b {
template <typename T>
using Node = robotics::Node<T>;
using JoyStick2D = robotics::JoyStick2D;
extern robotics::logger::Logger logger;

template <typename T>
using Muxer = robotics::filter::Muxer<T>;
}  // namespace nhk2024b
