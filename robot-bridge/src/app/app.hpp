#pragma once

#include "can_servo.hpp"
#include "ikako_robomas_bus.hpp"
#include "rohm_md.hpp"

namespace nhk2024b::robot2 {

class Actuators {
 public:
  struct Config {
    PinName can_1_rd;
    PinName can_1_td;
  };

 public:
  ikarashiCAN_mk2 can;

 public:
  common::CanServoBus can_servo;
  common::IkakoRobomasBus ikako_robomas;
  common::RohmMD rohm_md;

  Actuators(Config config)
      : can(config.can_1_rd, config.can_1_td, 0, (int)1E6),
        can_servo(can, 1),
        ikako_robomas(can),
        rohm_md(can, 2) {}

  void Init() { can.read_start(); }

  int Read() {
    ikako_robomas.Read();
    // rohm_md.Read();
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

    ret = rohm_md.Send();
    if (ret != 1) {
      // logger.Error("IkakoRobomasBus::Write failed");
      return -3;
    }

    return 0;
  }

  void Tick() {
    can.reset();

    ikako_robomas.Tick();
    ikako_robomas.Update();
  }
};
}  // namespace nhk2024b::robot2