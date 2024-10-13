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

class Test {
  using Actuators = nhk2024b::robot3::Actuators;

  Actuators actuators{(Actuators::Config){
      // Right Motor (M3)
      .move_motor_fin = GPIO_NUM_27,
      .move_motor_rin = GPIO_NUM_14,

      // Left Motor (M4)
      // .move_motor_fin = GPIO_NUM_27,
      // .move_motor_rin = GPIO_NUM_14,

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

 public:
  void Init() {
    logger.Info("Init");
    // puropo.stick1 >> robot.ctrl_stick_move;
    // puropo.button1 >> robot.ctrl_button_arm_open;
    // puropo.button2 >> robot.ctrl_button_arm_close;
    // puropo.button3 >> robot.ctrl_button_arm_up;
    // puropo.button4 >> robot.ctrl_button_arm_down;
    //
    // puropo.button5.SetChangeCallback([this](bool btn) {
    //   ctrl_emc = btn ? 0 : 1;  // pressed = 0 (stop actuators)
    //   UpdateEMC();
    // });

    robot.out_move >> actuators.move_motor;
    robot.out_arm_elevation >> actuators.arm_elevation_motor;
    robot.out_arm_expansion >> actuators.arm_expansion_motor;
    robot.LinkController();

    // puropo.button5.SetValue(false);

    logger.Info("Init done");
  }

  void Main() {
    logger.Info("Main loop");

    float previous = 0;

    int i = 0;
    while (1) {
      if (i % 1000 == 0) logger.Info("Update");
      // puropo.Tick();

      if (i % 200 == 0) {
        logger.Info("Report");
        // logger.Info("  Stick: %f, %f; %f, %f",    //
        //             puropo.stick1.GetValue()[0],  //
        //             puropo.stick1.GetValue()[1],  //
        //             puropo.stick2.GetValue()[0],  //
        //             puropo.stick2.GetValue()[1]   //
        // );
        logger.Info("  output: %f %f %f",                      //
                    actuators.move_motor.GetValue(),           //
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