#include <cinttypes>
// #include <mbed.h>

// #include "identify.h"
// #include "app.hpp"

// #include <mbed-robotics/simple_can.hpp>
#include <ikako_mdc/ikako_mdc.hpp>
#include <robotics/network/simple_can.hpp>
#include <robotics/network/uart_stream.hpp>
#include <logger/logger.hpp>
#include <robotics/network/fep/fep_driver.hpp>
#include <robotics/driver/dout.hpp>
#include <nhk2024b/fep.hpp>
#include "robot1-main.hpp"
#include <nhk2024b/controller_network.hpp>

class App {
  static inline robotics::logger::Logger logger{"robot1.app", "Robot1App"};
  DigitalOut emc{PC_0};

  nhk2024b::ControllerNetwork ctrl_net;
  nhk2024b::robot1::Controller *ctrl;

  bool emc_ctrl = false;
  bool emc_conn = true;

  nhk2024b::robot1::Refrige robot;
  ikarashiCAN_mk2 ican{PB_8, PB_9, 0, (int)1e6};

  robotics::registry::ikakoMDC mdc0;
  robotics::assembly::MotorPair<float> &motor0;
  robotics::assembly::MotorPair<float> &motor1;
  robotics::assembly::MotorPair<float> &motor2;
  robotics::assembly::MotorPair<float> &motor3;

  robotics::registry::ikakoMDC mdc1;
  robotics::assembly::MotorPair<float> &collector;
  robotics::assembly::MotorPair<float> &unlock;
  robotics::assembly::MotorPair<float> &brake;

  void UpdateEMC() {
    bool emc_state = emc_ctrl && emc_conn;
    emc.write(emc_state);
  }

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

      UpdateEMC();
    });

    ctrl->buttons.SetChangeCallback([this](DPad dpad) {
      robot.ctrl_brake.SetValue(dpad & DPad::kLeft);
      robot.ctrl_brake_back.SetValue(dpad & DPad::kRight);
      robot.ctrl_collector.SetValue(dpad & DPad::kDown);
      robot.ctrl_unlock.SetValue(dpad & DPad::kUp);
    });

    ctrl->emc.SetChangeCallback([this](bool btn) {
      emc_ctrl = emc_ctrl ^ btn;

      UpdateEMC();
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

    emc.write(1);
    ican.read_start();

    logger.Info("Init - Done");
  }

  void Main() {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    logger.Info("Main loop");
    int i = 0;

    Timer timer;
    timer.reset();
    timer.start();

    while (true) {
      auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          timer.elapsed_time())
                          .count();
      auto delta_s = delta_ms / 1000.0;
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

int main_switch() { return main0_alt0(); }