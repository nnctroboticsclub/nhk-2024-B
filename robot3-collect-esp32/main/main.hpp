#include <cinttypes>

#include <logger/logger.hpp>
#include <robotics/driver/dout.hpp>

#include "app.hpp"
#include "collect.hpp"
#include <robotics/timer/timer.hpp>

#include "DS4Controller.h"

robotics::logger::Logger logger{"   app   ", "app"};

template <typename T>
using Node = robotics::node::Node<T>;

using robotics::system::Timer;
using robotics::types::JoyStick2D;

class BTDS4Controller {
  static bool initialized_hw;

  static DS4Controller DS4Instance;
  DS4DataExt LastestState;
  unsigned long lastOutputReport = 0;

 public:
  Node<JoyStick2D> stick1;
  Node<JoyStick2D> stick2;
  Node<bool> button_squ;
  Node<bool> button_cro;
  Node<bool> button_cir;
  Node<bool> button_tri;

  Node<bool> button_shr;
  Node<bool> button_opt;

  BTDS4Controller() {
    if (!initialized_hw) {
      initialized_hw = true;
      BluetoothHID.Init();
      BluetoothHID.RegisterDevice(&BTDS4Controller::DS4Instance);
      BluetoothHID.ScanAndConnectHIDDevice();
    }
  }

  void Tick() {
    if (DS4Instance.GetState() != BTHIDState::Connected) return;
    if (!DS4Instance.HasNewReport()) return;

    auto reporting_state = DS4Instance.GetReportingState();
    if (reporting_state == DS4ReportingState::None) {
      return;
    } else if (reporting_state == DS4ReportingState::Standard) {
      DS4Instance.SetExtendedReporting();
      vTaskDelay(50 / portTICK_PERIOD_MS);
      return;
    }

    // reporting_state = Extended
    DS4Instance.GetLatestInputReport(&LastestState);

    /* DS4Instance.SetLED(LastestState.LeftX, LastestState.LeftY,
                       LastestState.RightX, false);
    DS4Instance.SetRumble(LastestState.LT, LastestState.RT, false);
    DS4Instance.UpdateState(); */

    stick1.SetValue(
        {(float)LastestState.LeftX / 255, (float)LastestState.LeftY / 255});
    stick2.SetValue(
        {(float)LastestState.RightX / 255, (float)LastestState.RightY / 255});

    button_squ.SetValue(LastestState.Buttons.square);
    button_cro.SetValue(LastestState.Buttons.cross);
    button_cir.SetValue(LastestState.Buttons.circle);
    button_tri.SetValue(LastestState.Buttons.triangle);

    button_shr.SetValue(LastestState.Buttons.share);
    button_opt.SetValue(LastestState.Buttons.options);
  }
};

bool BTDS4Controller::initialized_hw = false;
DS4Controller BTDS4Controller::DS4Instance;

class Test {
  using Actuators = nhk2024b::robot3::Actuators;

  Actuators actuators{(Actuators::Config){
      // Left Motor (M4)
      .move_l_motor_fin = GPIO_NUM_27,
      .move_l_forward_channel = LEDC_CHANNEL_0,
      .move_l_motor_rin = GPIO_NUM_14,
      .move_l_reverse_channel = LEDC_CHANNEL_1,

      // Right Motor (M3)
      .move_r_motor_fin = GPIO_NUM_27,
      .move_r_forward_channel = LEDC_CHANNEL_2,
      .move_r_motor_rin = GPIO_NUM_14,
      .move_r_reverse_channel = LEDC_CHANNEL_3,

      // Elevation Motor (M2)
      .arm_elevation_motor_fin = GPIO_NUM_25,
      .arm_elevation_motor_rin = GPIO_NUM_26,

      // Expansion Motor (M1)
      .arm_expansion_motor_fin = GPIO_NUM_32,
      .arm_expansion_motor_rin = GPIO_NUM_33,

      // LEDC
      .ledc_timer = LEDC_TIMER_0,
  }};

  nhk2024b::robot3::Robot robot{};

  bool ctrl_emc = false;
  bool hard_emc = false;

  BTDS4Controller ctrl;

 public:
  void Init() {
    logger.Info("Init");
    ctrl.stick1 >> robot.ctrl_stick_move;
    ctrl.button_cir >> robot.ctrl_button_arm_open;
    ctrl.button_squ >> robot.ctrl_button_arm_close;
    ctrl.button_tri >> robot.ctrl_button_arm_up;
    ctrl.button_cro >> robot.ctrl_button_arm_down;

    robot.out_move_l >> actuators.move_l_motor;
    robot.out_move_r >> actuators.move_r_motor;
    robot.out_arm_elevation >> actuators.arm_elevation_motor;
    robot.out_arm_expansion >> actuators.arm_expansion_motor;
    robot.LinkController();

    ctrl.button_shr.SetValue(false);

    logger.Info("Init done");
  }

  void Main() {
    logger.Info("Main loop");

    float previous = 0;

    int i = 0;
    while (1) {
      if (i % 1000 == 0) logger.Info("Update");
      ctrl.Tick();

      if (i % 200 == 0) {
        logger.Info("Report");
        logger.Info("  Stick: %f, %f; %f, %f",  //
                    ctrl.stick1.GetValue()[0],  //
                    ctrl.stick1.GetValue()[1],  //
                    ctrl.stick2.GetValue()[0],  //
                    ctrl.stick2.GetValue()[1]   //
        );
        logger.Info("  output: %f %f %f %f",                   //
                    actuators.move_l_motor.GetValue(),         //
                    actuators.move_r_motor.GetValue(),         //
                    actuators.arm_elevation_motor.GetValue(),  //
                    actuators.arm_expansion_motor.GetValue()   //
        );
        logger.Info("  (emc_ctrl = %d) & (hard_emc = %d) -> (emc_out = %d)",
                    ctrl_emc, hard_emc, ctrl_emc & hard_emc);
      }
      i += 1;
      vTaskDelay(1 / portTICK_PERIOD_MS);
    }
  }
};

int main_0() {
  xTaskCreate(
      [](void *) {
        auto test = std::make_unique<Test>();

        test->Init();
        test->Main();
      },
      "Main", 8192, nullptr, 1, nullptr);

  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

int main_switch() {
  printf("main() started\n");
  printf("Build: " __DATE__ " - " __TIME__ "\n");

  robotics::logger::core::Init();

  main_0();
  return 0;
}