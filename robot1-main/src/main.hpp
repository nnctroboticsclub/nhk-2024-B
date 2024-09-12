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

robotics::logger::Logger logger{"Robot1App", "robot1.app"};

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
  DigitalOut emc{PA_15};

  /*
  using PS4Con = PseudoController;
  PS4Con ps4;
  /*/
  using PS4Con = nhk2024b::ps4_con::PS4Con;
  PS4Con ps4{PC_6, PC_7, 115200};
  //*/

  bool emc_state = true;

  nhk2024b::robot1::Refrige robot;
  ikarashiCAN_mk2 ican{PB_5, PB_6, 0, (int)1e6};  // TODO: Fix this
  robotics::registry::ikakoMDC mdc0;
  robotics::assembly::MotorPair<float> &motor0;
  robotics::assembly::MotorPair<float> &motor1;
  robotics::assembly::MotorPair<float> &motor2;
  robotics::assembly::MotorPair<float> &motor3;
  robotics::registry::ikakoMDC mdc1;
  robotics::assembly::MotorPair<float> &collector;
  robotics::assembly::MotorPair<float> &unlock;
  robotics::assembly::MotorPair<float> &brake;
  robotics::assembly::MotorPair<float> &turning_r;
  robotics::assembly::MotorPair<float> &turning_l;

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
        brake(this->mdc1.GetNode(2)),
        turning_r(this->mdc1.GetNode(3)),
        turning_l(this->mdc1.GetNode(4)){}

  void Init() {
    using nhk2024b::ps4_con::DPad;
    using nhk2024b::ps4_con::Buttons;

    logger.Info("Init");

    ps4.dpad.SetChangeCallback([this](DPad dpad) {
      robot.ctrl_brake.SetValue(dpad & DPad::kLeft);
      robot.ctrl_collector.SetValue(dpad & DPad::kRight);
      robot.ctrl_unlock.SetValue(dpad & DPad::kUp);
    });

    ps4.button_share.SetChangeCallback([this](bool btn){
      emc_state = emc_state ^ btn;
      emc.write(emc_state ? 1:0);
    });

    ps4.trigger_r >> robot.ctrl_turning_right;//同じ方なら値渡しはこっちのほうがシンプルにできる
    ps4.trigger_l >> robot.ctrl_turning_left;//同じ方なら値渡しはこっちのほうがシンプルにできる

    ps4.stick_left >> robot.ctrl_move;

    robot.LinkController();

    robot.out_motor1 >> motor0.GetMotor();
    robot.out_motor2 >> motor1.GetMotor();
    robot.out_motor3 >> motor2.GetMotor();
    robot.out_motor4 >> motor3.GetMotor();

    robot.out_unlock >> unlock.GetMotor();
    robot.out_collector >> collector.GetMotor();
    robot.out_brake >> brake.GetMotor();
    robot.out_turning_right >> turning_r.GetMotor();
    robot.out_turning_left >> turning_l.GetMotor();

    ps4.Init();
    emc.write(1);
    ican.read_start();

    logger.Info("Init - Done");
  }

  void Main() {
    logger.Info("Main loop");
    int i = 0;
    while (1) {
      robot.Update(0.001);

      ps4.Update();

      ican.reset();

      mdc0.Tick();
      mdc1.Tick();

      int actuator_errors = 0;
      if (mdc0.Send() == 0) actuator_errors |= 1;
      if (mdc1.Send() == 0) actuator_errors |= 2;

      if (i % 100 == 0) {
        auto stick = ps4.stick_left.GetValue();
        logger.Info("Status");
        logger.Info("  actuator_errors: %d", actuator_errors);
        logger.Info("Report");
        logger.Info("  c %lf %lf", stick[0], stick[1]);
      }
      i += 1;
      ThisThread::sleep_for(1ms);
    }
  }
};

int main0_alt0() {
  auto test = new App();

  test->Init();
  test->Main();

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