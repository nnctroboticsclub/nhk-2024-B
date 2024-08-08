#include <cinttypes>
// #include <mbed.h>

// #include "identify.h"
// #include "app.hpp"

// #include <mbed-robotics/simple_can.hpp>
#include <robotics/network/fep/fep_driver.hpp>
#include <robotics/platform/dout.hpp>
#include <mbed-robotics/uart_stream.hpp>

#include <robotics/logger/logger.hpp>

#include <nhk2024b/fep_ps4_con.hpp>
#include "collect.hpp"

#include "app.hpp"

robotics::logger::Logger logger{"   app   ", "app"};

void InitFEP() {
  robotics::network::UARTStream uart{PC_6, PC_7, 115200};
  robotics::driver::Dout rst{PC_9};
  robotics::driver::Dout ini{PC_8};
  robotics::network::fep::FEPDriver fep_drv{uart, rst, ini};

  robotics::logger::SuppressLogger("rxp.fep.nw");
  robotics::logger::SuppressLogger("st.fep.nw");
  robotics::logger::SuppressLogger("sr.fep.nw");

  fep_drv.AddConfiguredRegister(0, 21);
  fep_drv.AddConfiguredRegister(1, 0xF0);
  fep_drv.AddConfiguredRegister(7, 0x22);
  fep_drv.AddConfiguredRegister(8, 0x2E);
  fep_drv.AddConfiguredRegister(9, 0x3A);
  fep_drv.ConfigureBaudrate(robotics::network::fep::FEPBaudrate(
      robotics::network::fep::FEPBaudrateValue::k115200));

  {
    auto result = fep_drv.Init();
    if (!result.IsOk()) {
      logger.Error("Failed to init FEP Driver: %s",
                   result.UnwrapError().c_str());
    }
  }
}

class Test {
  using PS4Con = nhk2024b::ps4_con::PS4Con;
  using Actuators = nhk2024b::robot3::Actuators;

  Actuators actuators{(Actuators::Config){
      .move_motor_l_fin = PB_10,
      .move_motor_l_rin = PB_2,
      .move_motor_r_fin = PB_9,
      .move_motor_r_rin = PB_8,

      .arm_elevation_motor_fin = PA_11,
      .arm_elevation_motor_rin = PA_10,

      .arm_extension_motor_fin = PA_9,
      .arm_extension_motor_rin = PA_8,
  }};
  PS4Con ps4{PC_6, PC_7, 115200};

  nhk2024b::robot3::Robot robot{};

 public:
  void Init() {
    logger.Info("Init");
    ps4.stick_left >> robot.ctrl_stick_rotate;
    ps4.stick_right >> robot.ctrl_stick_forward_back;
    ps4.button_circle >> robot.ctrl_button_arm_open;
    ps4.button_square >> robot.ctrl_button_arm_close;
    ps4.button_triangle >> robot.ctrl_button_arm_up;
    ps4.button_cross >> robot.ctrl_button_arm_down;

    robot.out_move_left >> actuators.move_motor_l;
    robot.out_move_right >> actuators.move_motor_r;
    robot.out_arm_elevation >> actuators.arm_elevation_motor;
    robot.out_arm_extension >> actuators.arm_extension_motor;

    robot.LinkController();

    ps4.Init();

    logger.Info("Init done");
  }

  void Main() {
    logger.Info("Main loop");
    int i = 0;
    while (1) {
      if (i % 1000 == 0) logger.Info("Update");
      ps4.Update();

      if (i % 100 == 0) {
        logger.Info("Report");
        logger.Info("  Stick: %f, %f", robot.ctrl_stick_rotate.GetValue()[0],
                    robot.ctrl_stick_rotate.GetValue()[1]);
        logger.Info("  output: %f %f %f %f", robot.out_move_left.GetValue(),
                    robot.out_move_right.GetValue(),
                    robot.out_arm_elevation.GetValue(),
                    robot.out_arm_extension.GetValue());
      }
      i += 1;
      ThisThread::sleep_for(1ms);
    }
  }
};

int main_0() {
  Thread thread{osPriorityNormal, 8192, nullptr, "Main"};
  thread.start([]() {
    InitFEP();

    auto test = new Test();

    test->Init();
    test->Main();
  });

  while (1) {
    ThisThread::sleep_for(100s);
  }

  return 0;
}

int main_1() { return 0; }

int main_2() { return 0; }

int main_3() { return 0; }

int main_pro() {
  /* App::Config config{        //
                     .com =  //
                     {
                         .can =
                             {
                                 .id = CAN_ID,
                                 .freqency = (int)1E6,
                                 .rx = PB_8,
                                 .tx = PB_9,
                             },
                         .driving_can =
                             {
                                 .rx = PB_5, // PB_5,
                                 .tx = PB_6, // PB_6,
                             },
                         .i2c =
                             {
                                 .sda = PC_9,
                                 .scl = PA_8,
                             },

                         .value_store_config = {},
                     },
                     .can1_debug = false};


App app(config);
  app.Init();

  while (1) {
    ThisThread::sleep_for(100s);
  }

  return 0; */
}

int main_switch() {
  printf("main() started\n");
  printf("Build: " __DATE__ " - " __TIME__ "\n");

  robotics::logger::Init();

  main_0();
  return 0;
}
