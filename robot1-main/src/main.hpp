#include <cinttypes>
// #include <mbed.h>

// #include "identify.h"
// #include "app.hpp"

// #include <mbed-robotics/simple_can.hpp>
#include <mbed-robotics/ikako_mdc.hpp>
#include <mbed-robotics/simple_can.hpp>
#include <mbed-robotics/uart_stream.hpp>
#include <nhk2024b/fep_ps4_con.hpp>
#include <robotics/logger/logger.hpp>
#include <robotics/network/fep/fep_driver.hpp>
#include <robotics/platform/dout.hpp>
#include <nhk2024b/fep.hpp>
#include "robot1-main.hpp"
#include <nhk2024b/test_ps4_fep.hpp>

#include <nhk2024b/controller_network.hpp>

robotics::logger::Logger logger{"robot1.app", "Robot1App"};

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
  DigitalOut emc{PC_0};

  nhk2024b::ControllerNetwork ctrl_net;
  nhk2024b::robot1::Controller *ctrl;

  bool emc_ctrl = false;
  bool emc_conn = true;

  nhk2024b::robot1::Refrige robot;
  ikarashiCAN_mk2 ican{PB_8, PB_9, 0, (int)1e6};  // TODO: Fix this

  robotics::registry::ikakoMDC mdc0;
  robotics::assembly::MotorPair<float> &motor0;
  robotics::assembly::MotorPair<float> &motor1;
  robotics::assembly::MotorPair<float> &motor2;
  robotics::assembly::MotorPair<float> &motor3;

  robotics::registry::ikakoMDC mdc1;
  robotics::assembly::MotorPair<float> &collector;
  robotics::assembly::MotorPair<float> &unlock;
  robotics::assembly::MotorPair<float> &brake;

 public:
  App()
      : mdc0(&ican, 9),
        motor0(this->mdc0.GetNode(0)),
        motor1(this->mdc0.GetNode(1)),
        motor2(this->mdc0.GetNode(2)),
        motor3(this->mdc0.GetNode(3)),

        mdc1(&ican, 6),
        collector(this->mdc1.GetNode(0)),
        unlock(this->mdc1.GetNode(1)),
        brake(this->mdc1.GetNode(2)) {}

  void Init() {
    using nhk2024b::ps4_con::Buttons;
    using nhk2024b::ps4_con::DPad;

    logger.Info("Init");

    ctrl_net.Init(0x0011);
    ctrl = ctrl_net.ConnectToPipe1();

    ctrl_net.keep_alive->connection_available.SetChangeCallback([this](bool v) {
      emc_conn = v;

      emc.write(emc_conn & emc_ctrl);
    });

    ctrl->buttons.SetChangeCallback([this](DPad dpad) {
      robot.ctrl_brake.SetValue(dpad & DPad::kLeft);
      robot.ctrl_brake_back.SetValue(dpad & DPad::kRight);
      robot.ctrl_collector.SetValue(dpad & DPad::kDown);
      robot.ctrl_unlock.SetValue(dpad & DPad::kUp);
    });

    ctrl->emc.SetChangeCallback([this](bool btn) {
      emc_ctrl = emc_ctrl ^ btn;

      emc.write(emc_ctrl & emc_conn);
    });

    ctrl->rotation_cw >>
        robot
            .ctrl_turning_right;  // 同じ方なら値渡しはこっちのほうがシンプルにできる
    ctrl->rotation_ccw >>
        robot
            .ctrl_turning_left;  // 同じ方なら値渡しはこっちのほうがシンプルにできる

    ctrl->move >> robot.ctrl_move;

    robot.LinkController();

    robot.out_motor1 >> motor0.GetMotor();
    robot.out_motor2 >> motor1.GetMotor();
    robot.out_motor3 >> motor2.GetMotor();
    robot.out_motor4 >> motor3.GetMotor();

    robot.out_unlock >> unlock.GetMotor();

    robot.out_collector >> collector.GetMotor();
    robot.out_brake >> brake.GetMotor();

    // ps4.Init();
    emc.write(1);
    ican.read_start();

    logger.Info("Init - Done");
  }

  void Main() {
    logger.Info("Main loop");
    int i = 0;

    Timer timer;
    timer.reset();
    timer.start();

    while (1) {
      float delta_s = timer.read_ms() / 1000.0;
      timer.reset();

      robot.Update(delta_s);
      ctrl_net.keep_alive->Update(delta_s);

      ican.reset();

      mdc0.Tick();
      mdc1.Tick();

      int actuator_errors = 0;
      if (mdc0.Send() == 0) actuator_errors |= 1;
      if (mdc1.Send() == 0) actuator_errors |= 2;

      if (i % 200 == 0 && false) {
        auto stick = ctrl->move.GetValue();
        logger.Info("Status");
        logger.Info("  actuator_errors: %d", actuator_errors);
        logger.Info("Report");
        logger.Info("  c %+6.3lf %+6.3lf", stick[0], stick[1]);
        logger.Info("  o: m:   %+6.3lf   %+6.3lf   %+6.3lf   %+6.3lf",
                    motor0.GetMotor().GetValue(), motor1.GetMotor().GetValue(),
                    motor2.GetMotor().GetValue(), motor3.GetMotor().GetValue());
        logger.Info("     u: c;%+6.3lf u;%+6.3lf b;%+6.3lf",
                    collector.GetMotor().GetValue(),
                    unlock.GetMotor().GetValue(), brake.GetMotor().GetValue());
      }
      i += 1;
      ThisThread::sleep_for(1ms);
    }
  }
};

int main0_alt0() {
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

  // nhk2024b::test::test_ps4_fep();
  return main0_alt0();
}