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

#include "robot1-main.hpp"

robotics::logger::Logger logger{"Robot2App", "robot2.app"};

void InitFEP() {
  robotics::network::UARTStream uart{PC_6, PC_7, 115200};
  robotics::driver::Dout rst{PC_9};
  robotics::driver::Dout ini{PC_8};
  robotics::network::fep::FEPDriver fep_drv{uart, rst, ini};

  // robotics::logger::SuppressLogger("rxp.fep.nw");
  // robotics::logger::SuppressLogger("st.fep.nw");
  // robotics::logger::SuppressLogger("sr.fep.nw");

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
  DigitalOut emc{PA_15};

  /*
  using PS4Con = PseudoController;
  PS4Con ps4;
  /*/
  using PS4Con = nhk2024b::ps4_con::PS4Con;
  PS4Con ps4{PC_6, PC_7, 115200};
  //*/

  nhk2024b::robot1::Refrige robot;
  ikarashiCAN_mk2 ican{PB_5, PB_6, (int)1e6};  // TODO: Fix this
  robotics::registry::ikakoMDC mdc0;
  robotics::assembly::MotorPair<float> &motor0;
  robotics::assembly::MotorPair<float> &motor1;
  robotics::assembly::MotorPair<float> &motor2;
  robotics::assembly::MotorPair<float> &motor3;
  robotics::registry::ikakoMDC mdc1;
  robotics::assembly::MotorPair<float> &collector;
  robotics::assembly::MotorPair<float> &lock;
  robotics::assembly::MotorPair<float> &lock_back;
  robotics::assembly::MotorPair<float> &brake;


 public:
  App()
      : mdc0(&ican, 0),
        motor0(this->mdc0.GetNode(0)),
        motor1(this->mdc0.GetNode(1)),
        motor2(this->mdc0.GetNode(2)),
        motor3(this->mdc0.GetNode(3)),
        mdc1(&ican, 1),
        collector(this->mdc1.GetNode(0)),
        lock(this->mdc1.GetNode(1)),
        lock_back(this->mdc1.GetNode(2)),
        brake(this->mdc1.GetNode(3)) {}

  void Init() {
    using nhk2024b::ps4_con::DPad;
    logger.Info("Init");

    InitFEP();

    ps4.dpad.SetChangeCallback([this](DPad dpad) {
      robot.ctrl_lock.SetValue(dpad & DPad::kUp);
      robot.ctrl_lock_back.SetValue(dpad & DPad::kDown);
      robot.ctrl_brake.SetValue(dpad & DPad::kLeft);
      robot.ctrl_collector.SetValue(dpad & DPad::kRight);
    });

    ps4.stick_left >> robot.ctrl_move;

    robot.out_motor1 >> motor0.GetMotor();
    robot.out_motor2 >> motor1.GetMotor();
    robot.out_motor3 >> motor2.GetMotor();
    robot.out_motor4 >> motor3.GetMotor();

    robot.out_brake >> lock.GetMotor(); 
    robot.out_brake >> collector.GetMotor(); 
    robot.out_brake >> lock_back.GetMotor(); 
    robot.out_brake >> brake.GetMotor(); 

    emc.write(1);

    logger.Info("Init - Done");
  }

  void Main() {
    logger.Info("Main loop");
    int i = 0;
    while (1) {
      ps4.Update();

      if (i % 100 == 0) {
        logger.Info("Status");
        logger.Info("  None");
        logger.Info("Report");
        logger.Info("  None");
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

int main_switch() {return main0_alt0();}