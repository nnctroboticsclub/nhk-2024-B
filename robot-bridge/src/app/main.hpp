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
  nhk2024b::common::Rohm1chMD *move_l;
  nhk2024b::common::Rohm1chMD *move_r;
  nhk2024b::common::Rohm1chMD *deploy;

  nhk2024b::ControllerNetwork ctrl_net;
  nhk2024b::robot2::Controller *ctrl;

  Robot robot;

  PwmOut led0{PA_6};
  PwmOut led1{PA_7};
  DigitalOut can_send_failed{PA_4};

  bool emc_ctrl = true;
  bool emc_conn = true;

 public:
  App() {}

  void Init() {
    logger.Info("Init - Ctrl");

    ctrl_net.Init(0x0012);
    ctrl_net.keep_alive->connection_available.SetChangeCallback(
        [this](bool available) {
          emc_conn = available;
          emc.write(emc_ctrl & emc_conn);
        });

    ctrl = ctrl_net.ConnectToPipe2();

    logger.Info("Init - Link");

    ctrl->move >> robot.ctrl_move;
    ctrl->button_deploy >> robot.ctrl_deploy;
    ctrl->button_bridge_toggle >> robot.ctrl_bridge_toggle;
    ctrl->test_increase >> robot.ctrl_test_increment;
    ctrl->test_decrease >> robot.ctrl_test_decrement;
    ctrl->emc.SetChangeCallback([this](bool btn) {
      emc_ctrl ^= btn;
      emc.write(emc_ctrl & emc_conn);
    });
    robot.LinkController();

    logger.Info("Init - Actuator");

    robot.out_move_l >> actuators->move_l.in_velocity;
    robot.out_move_r >> actuators->move_r.in_velocity;
    robot.out_deploy >> actuators->deploy.in_velocity;

    robot.out_bridge_unlock_duty.SetChangeCallback([this](float duty) {
      actuators->servo_0.SetValue(102 + 85 * duty);
      actuators->servo_1.SetValue(177.8 - 85 * duty);
    });
    actuators->servo_0.SetValue(102);
    actuators->servo_1.SetValue(177.8);

    robot.out_unlock_duty.SetChangeCallback([this](float duty) {
      actuators->servo_2.SetValue(128 + 128 * duty);
      actuators->servo_3.SetValue(128 - 128 * duty);
    });
    actuators->servo_2.SetValue(128);
    actuators->servo_3.SetValue(128);

    logger.Info("Init - LED");
    actuators->move_l.in_velocity.SetChangeCallback(
        [this](float v) { led0.write(v); });
    actuators->move_r.in_velocity.SetChangeCallback(
        [this](float v) { led1.write(v); });

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

    while (1) {
      float current = timer.read_ms() / 1000.0;
      float delta_s = current - previous;
      previous = current;

      ctrl_net.keep_alive->Update(delta_s);

      actuators->Tick();

      actuators->Read();
      int status_actuators_send_ = actuators->Send();
      can_send_failed = status_actuators_send_ != 0;

      if (i % 100 == 0) {
        auto stick = ctrl->move.GetValue();
        logger.Info("Status");
        logger.Info("  actuators_send %d", status_actuators_send_);
        logger.Info("Report");
        logger.Info("  s %f, %f", stick[0], stick[1]);
        logger.Info("  b d%d, b%d", ctrl->button_deploy.GetValue(),
                    ctrl->button_bridge_toggle.GetValue());
        logger.Info("  o s %f %f | %f %f",  ///
                    actuators->servo_0.GetValue(),
                    actuators->servo_1.GetValue(),
                    actuators->servo_2.GetValue(),
                    actuators->servo_3.GetValue()  //
        );
        logger.Info("    m %f %f", actuators->move_l.in_velocity.GetValue(),
                    actuators->move_r.in_velocity.GetValue());

        // logger.Info("Network");
        /* {
          auto can1_ESR = actuators->can1.get_can_instance()
                              ->get_can()
                              ->CanHandle.Instance->ESR;
          auto can1_rec = (can1_ESR >> 24) & 0xff;
          auto can1_tec = (can1_ESR >> 16) & 0xff;
          auto can1_lec = (can1_ESR >> 4) & 7;
          auto can1_boff = (can1_ESR >> 2) & 1;
          auto can1_bpvf = (can1_ESR >> 1) & 1;
          auto can1_bwgf = (can1_ESR >> 0) & 1;

          logger.Info("  can1");
          logger.Info("    REC = %d, TEC = %d LEC = %d", can1_rec, can1_tec,
                      can1_lec);
          logger.Info("    Bus Off?: %d, Passive?: %d, Warning?: %d", can1_boff,
                      can1_bpvf, can1_bwgf);
        } */
      }
      i += 1;
      ThisThread::sleep_for(1ms);
    }
  }
};

int main_prod() {
  auto thread = robotics::system::Thread();
  thread.SetThreadName("App");
  thread.SetStackSize(2048 + 2048 + 2048);

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
  nhk2024b::common::RohmMDBus md_bus;
  auto md = md_bus.NewNode(&can, 1);

  can.read_start();
  md->in_velocity.SetValue(0.2);
  /* md.out_velocity.SetChangeCallback(
      [&logger](float v) { logger.Info("out_velocity: %f", v); });
  md.out_current.SetChangeCallback(
      [&logger](float v) { logger.Info("out_current: %f", v); });
  md.out_radian.SetChangeCallback(
      [&logger](float v) { logger.Info("out_radian: %f", v); }); */

  logger.Info("Start");

  while (1) {
    md->Read();
    md->Send();

    ThisThread::sleep_for(1ms);
  }
}

int main_switch() {
  robotics::logger::SuppressLogger("rxp.fep.nw");
  robotics::logger::SuppressLogger("st.fep.nw");
  robotics::logger::SuppressLogger("sr.fep.nw");

  // return test_rohm_1ch_md();
  return main_prod();
}