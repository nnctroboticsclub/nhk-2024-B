#pragma once

#include <atomic>

#include <mbed.h>

#include <robotics/utils/emc.hpp>
#include <robotics/node/digital_out.hpp>
#include <robotics/utils/neopixel.hpp>

#include "communication.hpp"
#include "bridge.hpp"

class App {
  class Impl;

 public:
  struct Config {
    Communication::Config com;
    nhk2024b::BridgeController::Config bridge_ctrl;

    bool can1_debug;
  };

  Impl* impl;

  App(Config& config);

  void Init();
};