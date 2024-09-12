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

    PinName can_2_rd;
    PinName can_2_td;
  };

 public:
  ikarashiCAN_mk2 can1;
  ikarashiCAN_mk2 can2;

 public:
  common::CanServoBus can_servo;
  common::IkakoRobomasBus ikako_robomas;
  common::Rohm1chMD rohm_md;

  Actuators(Config config)
      : can1(config.can_1_rd, config.can_1_td, 0, (int)1E6),
        can2(config.can_2_rd, config.can_2_td, 0, (int)1E6),
        can_servo(can2, 1),
        ikako_robomas(can1),
        rohm_md(can2, 2) {}

  void Init() {
    can1.read_start();
    can2.read_start();
  }

  int Read() {
    ikako_robomas.Read();
    rohm_md.Read();
    return 0;
  }

  int Send() {
    int errors = 0;
    int status;

    // status = can_servo.Send();  // Sends 020H CAN message
    // if (status == 0) {
    //   // logger.Error("CanServoBus::Send failed");
    //   errors |= 1;
    // }

    status = ikako_robomas.Write();  // Sends 1FF/200H CAN message
    if (status != 1) {
      // logger.Error("IkakoRobomasBus::Write failed");
      errors |= 2;
    }

    // ikako_robomas's sender uses this_id to specify the message id
    // for the next message to be sent, so we need to restore it.
    can1.set_this_id(0);
    can2.set_this_id(0);

    // status = rohm_md.Send();
    // if (status != 1) {
    //   // logger.Error("IkakoRobomasBus::Write failed");
    //   errors |= 4;
    // }

    return -errors;
  }

  void Tick() {
    can1.reset();
    can2.reset();

    ikako_robomas.Tick();
    ikako_robomas.Update();
  }
};
}  // namespace nhk2024b::robot2