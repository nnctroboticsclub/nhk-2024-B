#include <cinttypes>
// #include <mbed.h>

// #include "identify.h"
// #include "app.hpp"

// #include <mbed-robotics/simple_can.hpp>
#include <mbed-robotics/simple_can.hpp>
#include <mbed-robotics/uart_stream.hpp>
#include <nhk2024b/fep_ps4_con.hpp>
#include <robotics/logger/logger.hpp>
#include <robotics/network/fep/fep_driver.hpp>
#include <robotics/platform/dout.hpp>

#include <nhk2024b/fep.hpp>
#include <nhk2024b/controller_network.hpp>

#include "app.hpp"
#include "bridge.hpp"

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

  Actuators *actuators = new Actuators{(Actuators::Config){
      .can_1_rd = PB_5,
      .can_1_td = PB_6,
      .can_2_rd = PA_11,
      .can_2_td = PA_12,
  }};

  IkakoRobomasNode *move_l;
  IkakoRobomasNode *move_r;
  IkakoRobomasNode *deploy;

  nhk2024b::common::CanServo *servo0;
  nhk2024b::common::CanServo *servo1;

  nhk2024b::ControllerNetwork ctrl_net;
  nhk2024b::robot2::Controller *ctrl;

  Robot robot;

  PwmOut led0{PA_6};
  PwmOut led1{PA_7};

  float fb2 = 0;
  float fb3 = 0;
  int status_actuators_send_ = 0;

 public:
  App()
      : move_l(actuators->ikako_robomas.NewNode(2, new IkakoM3508(2))),
        move_r(actuators->ikako_robomas.NewNode(3, new IkakoM3508(3))),
        deploy(actuators->ikako_robomas.NewNode(4, new IkakoM3508(4))),
        servo0(actuators->can_servo.NewNode(0)),
        servo1(actuators->can_servo.NewNode(1)) {}

  void Init() {
    logger.Info("Init");

    ctrl_net.Init();
    ctrl = ctrl_net.ConnectToPipe2();

    ctrl_net.keep_alive->connection_available.SetChangeCallback(
        [this](bool available) {
          if (available) {
            logger.Info("Connection available");
          } else {
            logger.Info("Connection lost");
          }
        });

    ctrl->move >> robot.ctrl_move;
    ctrl->button_deploy >> robot.ctrl_deploy;
    ctrl->button_bridge_toggle >> robot.ctrl_bridge_toggle;

    robot.LinkController();

    robot.out_move_l >> move_l->velocity;
    robot.out_move_r >> move_r->velocity;
    robot.out_deploy >> actuators->rohm_md.in_velocity;

    ctrl->move.SetChangeCallback([this](robotics::types::JoyStick2D x) {
      led0.write((1 + x[0]) / 2);
      led1.write((1 - x[0]) / 2);
    });

    robot.out_unlock_duty.SetChangeCallback([this](float duty) {
      servo0->SetValue(102 + 85 * duty);
      servo1->SetValue(177.8 - 85 * duty);
    });
    servo0->SetValue(102);
    servo1->SetValue(177.8);

    // ps4.Propagate();

    emc.write(1);
    actuators->Init();

    logger.Info("Init - Done");
  }

  void Main() {
    logger.Info("Main loop");
    int i = 0;
    Timer timer;
    timer.reset();
    timer.start();

    while (1) {
      actuators->Read();

      float delta_s = timer.read_ms() / 1000.0;
      timer.reset();

      ctrl_net.keep_alive->Update(delta_s);

      status_actuators_send_ = actuators->Send();
      actuators->Tick();

      if (i % 100 == 0 && false) {
        auto stick = ctrl->move.GetValue();
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
};

int main_prod() {
  auto thread = robotics::system::Thread();
  thread.SetThreadName("App");
  thread.SetStackSize(8192);

  thread.Start([]() {
    auto test = new App();

    test->Init();
    test->Main();
  });

  return 0;
}

int main_switch() {
  robotics::logger::SuppressLogger("rxp.fep.nw");
  robotics::logger::SuppressLogger("st.fep.nw");
  robotics::logger::SuppressLogger("sr.fep.nw");

  // nhk2024b::InitFEP();

  return main_prod();
}