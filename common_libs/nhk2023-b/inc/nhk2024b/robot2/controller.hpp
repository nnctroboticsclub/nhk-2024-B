#pragma once

#include <robotics/logger/logger.hpp>
#include <robotics/node/node.hpp>
#include <robotics/network/ssp/ssp.hpp>
#include <robotics/network/ssp/value_store.hpp>
#include "../ps4_con.hpp"
#include "../types.hpp"

namespace nhk2024b::robot2 {
struct ControllerDataFrame {
  uint8_t move_x : 8;
  uint8_t move_y : 8;
  int emc : 1;
  int button_deploy : 1;
  int button_bridge_toggle : 1;
  int button_unassigned0 : 1;
  int button_unassigned1 : 1;
  int test_increase : 1;
  int test_decrease : 1;
} __attribute__ ((__packed__));
class Controller {
 public:
  Node<JoyStick2D> move;
  Node<bool> emc;
  Node<bool> button_deploy;
  Node<bool> button_bridge_toggle;
  Node<bool> button_unassigned0;
  Node<bool> button_unassigned1;
  Node<bool> test_increase;
  Node<bool> test_decrease;

  std::array<uint8_t, 3> Pack() {
    union {
      ControllerDataFrame df;
      uint8_t bytes[3];
    } df;
    df.df.move_x = (move.GetValue()[0] + 1) * 255 / 2;
    df.df.move_y = (move.GetValue()[1] + 1) * 255 / 2;
    df.df.emc = emc.GetValue();
    df.df.button_deploy = button_deploy.GetValue();
    df.df.button_bridge_toggle = button_bridge_toggle.GetValue();
    df.df.button_unassigned0 = button_unassigned0.GetValue();
    df.df.button_unassigned1 = button_unassigned1.GetValue();
    df.df.test_increase = test_increase.GetValue();
    df.df.test_decrease = test_decrease.GetValue();

    return std::array<uint8_t, 3>{
        df.bytes[0],
        df.bytes[1],
        df.bytes[2],
    };
  }

  void Unpack(std::array<uint8_t, 3> raw_data) {
    union {
      ControllerDataFrame df;
      uint8_t bytes[3];
    } df = {
        raw_data[0],
        raw_data[1],
        raw_data[2],
    };

    auto stick_x = (df.df.move_x / 255.0f - 0.5f) * 2;
    auto stick_y = (df.df.move_y / 255.0f - 0.5f) * 2;
    auto stick = robotics::types::JoyStick2D(stick_x, stick_y);
    move.SetValue(stick);

    emc.SetValue(df.df.emc);
    button_deploy.SetValue(df.df.button_deploy);
    button_bridge_toggle.SetValue(df.df.button_bridge_toggle);
    button_unassigned0.SetValue(df.df.button_unassigned0);
    button_unassigned1.SetValue(df.df.button_unassigned1);
    test_increase.SetValue(df.df.test_increase);
    test_decrease.SetValue(df.df.test_decrease);
  }
};
}  // namespace nhk2024b::robot2