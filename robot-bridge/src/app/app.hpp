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
  common::Rohm1chMD rohm_md;

  Actuators(Config config)
      : can(config.can_1_rd, config.can_1_td, 5, (int)1E6),
        can_servo(can, 1),
        ikako_robomas(can),
        rohm_md(can, 2) {}

  void Init() { can.read_start(); }

  int Read() {
    ikako_robomas.Read();
    rohm_md.Read();
    return 0;
  }

  int Send() {
    int errors = 0;

    int status = can_servo.Send();  // Sends 025H CAN message
    if (status == 0) {
      // logger.Error("CanServoBus::Send failed");
      errors |= 1;
    }

    status = ikako_robomas.Write();  // Sends 1FF/200H CAN message
    if (status != 1) {
      // logger.Error("IkakoRobomasBus::Write failed");
      errors |= 2;
    }

    // ikako_robomas's sender uses this_id to specify the message id
    // for the next message to be sent, so we need to restore it.
    can.set_this_id(0x05);

    status = rohm_md.Send();
    if (status != 1) {
      // logger.Error("IkakoRobomasBus::Write failed");
      errors |= 4;
    }

    return -errors;
  }

  void Tick() {
    can.reset();

    ikako_robomas.Tick();
    ikako_robomas.Update();
  }
};
}  // namespace nhk2024b::robot2