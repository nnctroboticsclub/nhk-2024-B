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

#include <ikako_m2006.h>

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

  DigitalOut emc{PC_0};

  Actuators *actuators = new Actuators{(Actuators::Config){
      .can_1_rd = PB_8,
      .can_1_td = PB_9,
      .can_2_rd = PB_5,
      .can_2_td = PB_6,
  }};

  nhk2024b::common::CanServo *servo0;
  nhk2024b::common::CanServo *servo1;
  nhk2024b::common::Rohm1chMD &move_l;
  nhk2024b::common::Rohm1chMD &move_r;
  nhk2024b::common::Rohm1chMD &deploy;

  nhk2024b::ControllerNetwork ctrl_net;
  nhk2024b::robot2::Controller *ctrl;

  Robot robot;

  PwmOut led0{PA_6};
  PwmOut led1{PA_7};
  DigitalOut can_send_failed{PA_4};

  float fb2 = 0;
  float fb3 = 0;
  int status_actuators_send_ = 0;
  bool emc_ctrl = true;
  bool emc_conn = true;

 public:
  App()
      : move_l(actuators->rohm_md.NewNode(2)),
        move_r(actuators->rohm_md.NewNode(3)),
        deploy(actuators->rohm_md.NewNode(4)),
        servo0(actuators->can_servo.NewNode(0)),
        servo1(actuators->can_servo.NewNode(1)) {}

  void Init() {
    logger.Info("Init");

    ctrl_net.Init(0x0012);
    ctrl = ctrl_net.ConnectToPipe2();

    ctrl->move >> robot.ctrl_move;
    ctrl->button_deploy >> robot.ctrl_deploy;
    ctrl->button_bridge_toggle >> robot.ctrl_bridge_toggle;
    ctrl->emc.SetChangeCallback([this](bool btn) {
      emc_ctrl ^= btn;
      emc.write(emc_ctrl & emc_conn);
    });

    ctrl_net.keep_alive->connection_available.SetChangeCallback(
        [this](bool available) {
          emc_conn = available;
          emc.write(emc_ctrl & emc_conn);
        });

    robot.LinkController();

    robot.out_move_l >> move_l.in_velocity;
    robot.out_move_r >> move_r.in_velocity;
    robot.out_deploy >> deploy.in_velocity;

    move_l.in_velocity.SetChangeCallback([this](float v) { led0.write(v); });
    move_r.in_velocity.SetChangeCallback([this](float v) { led1.write(v); });

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

    /* dummy4->velocity.SetValue(0);
    dummy5->velocity.SetValue(0);
    dummy6->velocity.SetValue(0);
    dummy7->velocity.SetValue(0); */

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
    float previous = 0;
    float scheduled_can1_reset = 0.1;

    while (1) {
      float current = timer.read_ms() / 1000.0;
      float delta_s = current - previous;
      previous = current;

      ctrl_net.keep_alive->Update(delta_s);

      /* if (scheduled_can1_reset < current) {
        logger.Info("\x1b[1;31m[Actuators] Resetting CAN1 Peripheral\x1b[m");
        __HAL_RCC_CAN1_FORCE_RESET();
        __HAL_RCC_CAN1_RELEASE_RESET();

        scheduled_can1_reset = current + 0.1;
      } */

      actuators->Read();
      status_actuators_send_ = actuators->Send();
      actuators->Tick();
      can_send_failed = status_actuators_send_ != 0;

      if (i % 100 == 0) {
        auto stick = ctrl->move.GetValue();
        logger.Info("Status");
        logger.Info("  actuators_send %d", status_actuators_send_);
        logger.Info("Report");
        logger.Info("  s %f, %f", stick[0], stick[1]);
        logger.Info("  b d%d, b%d", ctrl->button_deploy.GetValue(),
                    ctrl->button_bridge_toggle.GetValue());
        logger.Info("  o s %f %f", servo0->GetValue(), servo1->GetValue());
        logger.Info("    m %f %f", move_l.in_velocity.GetValue(),
                    move_r.in_velocity.GetValue());
        logger.Info("Network");
        logger.Info("  can1");
        {
          auto can1_ESR = actuators->can1.get_can_instance()
                              ->get_can()
                              ->CanHandle.Instance->ESR;
          auto can1_rec = (can1_ESR >> 24) & 0xff;
          auto can1_tec = (can1_ESR >> 16) & 0xff;
          auto can1_lec = (can1_ESR >> 4) & 7;
          auto can1_boff = (can1_ESR >> 2) & 1;
          auto can1_bpvf = (can1_ESR >> 1) & 1;
          auto can1_bwgf = (can1_ESR >> 0) & 1;

          logger.Info("    REC = %d, TEC = %d LEC = %d", can1_rec, can1_tec,
                      can1_lec);
          logger.Info("    Bus Off?: %d, Passive?: %d, Warning?: %d", can1_boff,
                      can1_bpvf, can1_bwgf);
        }
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

int test_rohm_1ch_md() {
  robotics::logger::Logger logger("rohm1chmd", "rohm1chmd");
  // tr
  ikarashiCAN_mk2 can(PB_8, PB_9, 0, (int)1E6);
  nhk2024b::common::Rohm1chMD md(can, 1);

  can.read_start();
  md.in_velocity.SetValue(0.2);
  md.out_velocity.SetChangeCallback(
      [&logger](float v) { logger.Info("out_velocity: %f", v); });
  md.out_current.SetChangeCallback(
      [&logger](float v) { logger.Info("out_current: %f", v); });
  md.out_radian.SetChangeCallback(
      [&logger](float v) { logger.Info("out_radian: %f", v); });

  logger.Info("Start");

  while (1) {
    md.Read();
    md.Send();

    ThisThread::sleep_for(1ms);
  }
}

int main_switch() {
  robotics::logger::SuppressLogger("rxp.fep.nw");
  robotics::logger::SuppressLogger("st.fep.nw");
  robotics::logger::SuppressLogger("sr.fep.nw");

  return test_rohm_1ch_md();
}