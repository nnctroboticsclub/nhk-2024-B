#pragma once

#include <atomic>

#include <mbed.h>

#include <robotics/utils/emc.hpp>
#include <robotics/node/digital_out.hpp>
#include <robotics/utils/neopixel.hpp>

#include "communication.hpp"

class App {
  class Impl;

 public:
  struct Config {
    Communication::Config com;

    bool can1_debug;
  };

  Impl* impl;

  App(Config& config);

  void Init();
};