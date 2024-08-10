#include <cinttypes>
// #include <mbed.h>

// #include "identify.h"
// #include "app.hpp"

// #include <mbed-robotics/simple_can.hpp>
#include <robotics/network/fep/fep_driver.hpp>
#include <robotics/platform/dout.hpp>
#include <mbed-robotics/uart_stream.hpp>

#include <robotics/logger/logger.hpp>
#include <mbed-robotics/simple_can.hpp>
#include <nhk2024b/fep_ps4_con.hpp>
#include "bridge.hpp"

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

class PseudoController {
  robotics::system::Thread thread;

 public:
  robotics::Node<robotics::types::JoyStick2D> stick_right;
  robotics::Node<bool> button_square;
  robotics::Node<bool> button_cross;
  robotics::Node<bool> button_circle;
  // robotics::Node<bool> button_triangle;

  PseudoController() {
    thread.SetThreadName("PseudoController");
    thread.Start([this]() {
      button_square.SetValue(false);
      while (1) {
        stick_right.SetValue({0, 0});
        button_cross.SetValue(false);
        button_circle.SetValue(false);
        // button_triangle.SetValue(false);

        ThisThread::sleep_for(100ms);

        stick_right.SetValue({0, 1});

        button_cross.SetValue(true);
        button_circle.SetValue(true);
        // button_triangle.SetValue(true);

        ThisThread::sleep_for(100ms);
      }
    });
  }

  void Update() {}
};

class App {
  using Actuators = nhk2024b::robot2::Actuators;
  using Robot = nhk2024b::robot2::Robot;

  DigitalOut emc{PA_15};

  using PS4Con = PseudoController;
  PS4Con ps4;

  // using PS4Con = nhk2024b::ps4_con::PS4Con;
  // PS4Con ps4{PC_6, PC_7, 115200};

  robotics::logger::Logger logger{"Robot2App", "robot2.app"};

  Actuators actuators{(Actuators::Config){
      .can_1_rd = PA_11,
      .can_1_td = PA_12,
  }};
  IkakoRobomasNode *move_l;
  IkakoRobomasNode *move_r;

  nhk2024b::common::CanServo *servo0;
  nhk2024b::common::CanServo *servo1;

  Robot robot;

 public:
  App()
      : move_l(actuators.ikako_robomas.NewNode(0)),
        move_r(actuators.ikako_robomas.NewNode(1)),
        servo0(actuators.can_servo.NewNode(0)),
        servo1(actuators.can_servo.NewNode(1)) {}

  void Init() {
    logger.Info("Init");

    // InitFEP();

    ps4.stick_right >> robot.ctrl_move;
    ps4.button_cross >> robot.ctrl_deploy;
    ps4.button_square >> robot.ctrl_test_unlock_dec;
    ps4.button_circle >> robot.ctrl_test_unlock_inc;

    robot.out_move_l >> move_l->velocity;
    robot.out_move_r >> move_r->velocity;
    robot.out_deploy;
    robot.out_unlock_duty.SetChangeCallback([this](float duty) {
      servo0->SetValue(127 + duty);
      servo1->SetValue(127 - duty);
    });

    emc.write(1);

    logger.Info("Init - Done");
  }

  void Main() {
    logger.Info("Main loop");
    int i = 0;
    while (1) {
      if (i % 1000 == 0) logger.Info("Update");
      ps4.Update();
      {
        int ret = actuators.Send();
        if (ret != 0) {
          // logger.Error("Actuators::Send failed [%d]", ret);
        }
      }

      // actuators.Tick();

      if (i % 100 == 0) {
        // auto stick = ps4.stick_right.GetValue();
        // logger.Info("Report");
        // logger.Info("  s %f, %f", stick[0], stick[1]);
        // logger.Info("  o %f %f %f %f", robot.out_deploy.GetValue(),
        //             robot.out_unlock_duty.GetValue(),
        //             robot.out_move_l.GetValue(),
        //             robot.out_move_r.GetValue());
      }
      i += 1;
      ThisThread::sleep_for(1ms);
    }
  }
};

int main_0() {
  Thread thread{osPriorityNormal, 8192, nullptr, "Main"};
  thread.start([]() {
    auto test = new App();

    test->Init();
    test->Main();
  });

  while (1) {
    ThisThread::sleep_for(100s);
  }

  return 0;
}

int main_1() {
  robotics::network::SimpleCAN can_{PA_11, PA_12, (int)1E6};
  logger.Info("Init");
  can_.Init();
  while (1) {
    ThisThread::sleep_for(50ms);
    auto ret = can_.Send(0x200, {1, 2, 3, 4});
    if (ret != 1) {
      logger.Error("Send failed [%d]", ret);
    }
  }
  return 0;
}

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

  main_1();
  return 0;
}
