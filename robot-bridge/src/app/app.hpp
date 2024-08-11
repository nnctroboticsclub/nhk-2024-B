#pragma once

#include "can_servo.hpp"
#include "ikako_robomas_bus.hpp"

namespace nhk2024b::robot2 {

class Actuators {
 public:
  struct Config {
    PinName can_1_rd;
    PinName can_1_td;
  };

 public:
  ikarashiCAN_mk2 can;
  Thread thread;

 public:
  common::CanServoBus can_servo;
  common::IkakoRobomasBus ikako_robomas;

  Actuators(Config config)
      : can(config.can_1_rd, config.can_1_td, 0, (int)1E6),
        can_servo(can, 1),
        ikako_robomas(can) {}

  void Init() {
    can.read_start();
    thread.start([&]() {
      while (1) {
        can.reset();
        ThisThread::sleep_for(10ms);
      }
    });
  }

  int Read() {
    ikako_robomas.Read();
    return 0;
  }

  int Send() {
    int ret = can_servo.Send();
    if (ret == 0) {
      // logger.Error("CanServoBus::Send failed");
      return -1;
    }
    ret = ikako_robomas.Write();
    if (ret != 1) {
      // logger.Error("IkakoRobomasBus::Write failed");
      return -2;
    }

    return 0;
  }

  void Tick() {
    ikako_robomas.Tick();
    ikako_robomas.Update();
  }
};
}  // namespace nhk2024b::robot2