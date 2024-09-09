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

robotics::logger::Logger logger{"robot2.app", "Robot2App"};

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

  /*
  using PS4Con = PseudoController;
  PS4Con ps4;
  /*/
  using PS4Con = nhk2024b::ps4_con::PS4Con;
  PS4Con ps4{PC_6, PC_7, 115200};
  //*/

  Actuators *actuators = new Actuators{(Actuators::Config){
      .can_1_rd = PB_5 /* PA_11 */,
      .can_1_td = PB_6 /* PA_12 */,
  }};
  IkakoRobomasNode *move_l;
  IkakoRobomasNode *move_r;

  nhk2024b::common::CanServo *servo0;
  nhk2024b::common::CanServo *servo1;

  Robot robot;

  int status_actuators_send_ = 0;

 public:
  App()
      : move_l(actuators->ikako_robomas.NewNode(1)),
        move_r(actuators->ikako_robomas.NewNode(2)),
        servo0(actuators->can_servo.NewNode(0)),
        servo1(actuators->can_servo.NewNode(1)) {}

  void Init() {
    logger.Info("Init");

    ps4.stick_right >> robot.ctrl_move;
    ps4.button_cross >> robot.ctrl_deploy;
    ps4.button_square >> robot.ctrl_test_unlock_dec;
    ps4.button_circle >> robot.ctrl_test_unlock_inc;

    robot.LinkController();

    robot.out_move_l >> move_l->velocity;
    robot.out_move_r >> move_r->velocity;
    // robot.out_deploy >> actuators->rohm_md.in_velocity;
    robot.out_unlock_duty.SetChangeCallback([this](float duty) {
      servo0->SetValue(127 + 127 * duty);
      servo1->SetValue(127 - 127 * duty);
    });

    servo0->SetValue(127);
    servo1->SetValue(127);

    move_l->velocity.SetValue(0);
    move_r->velocity.SetValue(0);

    ps4.Init();
    // ps4.Propagate();

    emc.write(1);
    actuators->Init();

    logger.Info("Init - Done");
  }

  void Main() {
    logger.Info("Main loop");
    int i = 0;
    while (1) {
      ps4.Update();
      actuators->Read();

      status_actuators_send_ = actuators->Send();

      actuators->Tick();

      if (i % 100 == 0) {
        auto stick = ps4.stick_right.GetValue();
        logger.Info("Status");
        logger.Info("  actuators_send %d", status_actuators_send_);
        logger.Info("Report");
        logger.Info("  s %f, %f", stick[0], stick[1]);
        logger.Info("  o %f %f", servo0->GetValue(), servo1->GetValue());
      }
      i += 1;
      ThisThread::sleep_for(1ms);
    }
  }

  void Test() {
    logger.Info("Main loop");
    int i = 0;
    while (1) {
      actuators->Read();

      servo0->SetValue(255 * 0.0);
      servo1->SetValue(255 * 0.2);

      status_actuators_send_ = actuators->Send();

      actuators->Tick();

      if (i % 100 == 0) {
        auto stick = ps4.stick_right.GetValue();
        logger.Info("Status");
        logger.Info("  actuators_send %d", status_actuators_send_);
        logger.Info("Report");
        logger.Info("  s %f, %f", stick[0], stick[1]);
        logger.Info("  o %f %f %f %f", robot.out_deploy.GetValue(),
                    robot.out_unlock_duty.GetValue(),
                    robot.out_move_l.GetValue(), robot.out_move_r.GetValue());
      }
      i += 1;
      ThisThread::sleep_for(1ms);
    }
  }
};

int main_prod() {
  auto test = new App();

  test->Init();
  test->Main();

  return 0;
}

int can_servo_test() {
  auto test = new App();

  test->Init();
  test->Test();

  return 0;
}

int main_switch() {
  robotics::logger::SuppressLogger("rxp.fep.nw");
  robotics::logger::SuppressLogger("st.fep.nw");
  robotics::logger::SuppressLogger("sr.fep.nw");

  return main_prod();
}