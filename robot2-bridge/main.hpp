#include <cinttypes>
// #include <mbed.h>

// #include "identify.h"
// #include "app.hpp"

#include <robotics/network/simple_can.hpp>
#include <robotics/network/uart_stream.hpp>
#include <logger/logger.hpp>
#include <robotics/network/fep/fep_driver.hpp>
#include <robotics/driver/dout.hpp>
#include <nhk2024b/fep.hpp>
#include <nhk2024b/controller_network.hpp>
#include <ikako_m2006.h>
#include "NHKPuropo.h"
#include "app.hpp"
#include "bridge.hpp"

robotics::logger::Logger logger{"robot2.app", "Robot2App"};

template <typename T>
using Node = robotics::node::Node<T>;

using robotics::types::JoyStick2D;

class PuropoController {
 public:
  NHK_Puropo puropo;

  Node<JoyStick2D> stick1;
  Node<JoyStick2D> stick2;
  Node<bool> button1;
  Node<bool> button2;
  Node<bool> button3;
  Node<bool> button4;
  Node<bool> button5;

  Node<bool> status;

  // コンストラクタ→初期化
  PuropoController(PinName tx, PinName rx) : puropo(tx, rx) {
    puropo.setup();
    stick1.SetValue(JoyStick2D{0, 0});
    stick2.SetValue(JoyStick2D{0, 0});
    button1.SetValue(false);
    button2.SetValue(false);
    button3.SetValue(false);
    button4.SetValue(false);
    button5.SetValue(false);
    status.SetValue(false);
  }

  // 毎ティック実行される関数
  void Tick() {
    // プロポの値を Node に格納
    // <xxx>.SetValue(<value>);

    puropo.update();

    auto stick1_value = JoyStick2D{-1 * puropo.get(4), puropo.get(2)};
    auto stick2_value = JoyStick2D{-1 * puropo.get(1), puropo.get(3)};
    stick1.SetValue(stick1_value);
    stick2.SetValue(stick2_value);
    button1.SetValue((puropo.get(5) + 1) / 2);
    button2.SetValue((puropo.get(6) + 1) / 2);
    button3.SetValue((puropo.get(7) + 1) /
                     2);  // Cボタンだけ真ん中に立てられるがそれはしないこと
    button4.SetValue((puropo.get(8) + 1) / 2);
    button5.SetValue((puropo.get(10) + 1) / 2);

    status.SetValue(this->puropo.is_ok());
  }
};

class App {
  using Actuators = nhk2024b::robot2::Actuators;
  using Robot = nhk2024b::robot2::Robot;

  DigitalOut emc{PB_7};

  Actuators *actuators = new Actuators{(Actuators::Config){
      .can_1_rd = PB_5,
      .can_1_td = PB_6,
      .can_2_rd = PB_8,
      .can_2_td = PB_9,
  }};

  nhk2024b::common::CanServo *servo0;
  nhk2024b::common::CanServo *servo1;
  nhk2024b::common::Rohm1chMD *move_l;
  nhk2024b::common::Rohm1chMD *move_r;
  nhk2024b::common::Rohm1chMD *deploy;

  // nhk2024b::ControllerNetwork ctrl_net;
  // nhk2024b::robot2::Controller *ctrl;
  PuropoController ctrl{PA_0, PA_1};

  Robot robot;

  PwmOut led0{PA_6};
  PwmOut led1{PA_7};
  DigitalOut can_send_failed{PA_4};

  bool emc_ctrl = true;
  bool emc_conn = true;

  void UpdateEMC() {
    bool emc_state = emc_ctrl & emc_conn;
    emc.write(emc_state);
  }

 public:
  App() {}

  void Init() {
    logger.Info("Init - Ctrl");

    /* ctrl_net.Init(0x0012);
    ctrl_net.keep_alive->connection_available.SetChangeCallback(
        [this](bool available) {
          emc_conn = available;
          UpdateEMC();
        });

    ctrl = ctrl_net.ConnectToPipe2(); */

    logger.Info("Init - Actuator");

    robot.out_move_l >> actuators->move_l.velocity;
    robot.out_move_r >> actuators->move_r.velocity;
    robot.out_deploy >> actuators->deploy.in_velocity;
    actuators->move_l.velocity.SetValue(0);
    actuators->move_r.velocity.SetValue(0);
    actuators->deploy.in_velocity.SetValue(0);

    robot.out_bridge_unlock_duty.SetChangeCallback([this](float duty) {
      actuators->servo_0.SetValue(102 + 85 * duty);
      actuators->servo_1.SetValue(177.8 - 85 * duty);
    });
    actuators->servo_0.SetValue(102);
    actuators->servo_1.SetValue(177.8);

    robot.out_unlock_duty.SetChangeCallback([this](float duty) {
      actuators->servo_2.SetValue(128 - 128 * duty);
      actuators->servo_3.SetValue(128 - 128 * duty);
    });
    actuators->servo_2.SetValue(128);
    actuators->servo_3.SetValue(128);

    logger.Info("Init - LED");
    actuators->move_l.velocity.SetChangeCallback(
        [this](float v) { led0.write(v); });
    actuators->move_r.velocity.SetChangeCallback(
        [this](float v) { led1.write(v); });

    logger.Info("Init - Link");

    ctrl.stick1 >> robot.ctrl_move;
    ctrl.button1 >> robot.ctrl_deploy;
    ctrl.button2 >> robot.ctrl_bridge_toggle;
    ctrl.button3 >> robot.ctrl_unlock;
    ctrl.button4.SetChangeCallback([this](bool btn) {
      emc_ctrl ^= btn;
      emc.write(emc_ctrl & emc_conn);
    });
    robot.LinkController();

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

    while (true) {
      float current = timer.read_ms() / 1000.0;
      float delta_s = current - previous;
      previous = current;

      // ctrl_net.keep_alive->Update(delta_s);
      ctrl.Tick();

      actuators->Tick();

      actuators->Read();
      int status_actuators_send_ = actuators->Send();
      can_send_failed = status_actuators_send_ != 0;

      if (i % 50 == 0 && false) {
        auto stick = ctrl.stick1.GetValue();
        logger.Info("Status");
        logger.Info("  actuators_send %d", status_actuators_send_);
        logger.Info("  ctrl.status [%d]", ctrl.status.GetValue());
        logger.Info("Report");
        logger.Info("  s %f, %f", stick[0], stick[1]);
        logger.Info("  b d%d, b%d", ctrl.button1.GetValue(),
                    ctrl.button2.GetValue());
        logger.Info("  o s %f %f | %f %f",  ///
                    actuators->servo_0.GetValue(),
                    actuators->servo_1.GetValue(),
                    actuators->servo_2.GetValue(),
                    actuators->servo_3.GetValue()  //
        );
        logger.Info("    m %f %f %f", actuators->move_l.velocity.GetValue(),
                    actuators->move_r.velocity.GetValue(),
                    actuators->deploy.in_velocity.GetValue());

        logger.Info("Network");
        {
          auto can = actuators->can1.get_can_instance()->get_can();
          auto esr = can->CanHandle.Instance->ESR;
          auto rec = (esr >> 24) & 0xff;
          auto tec = (esr >> 16) & 0xff;
          auto lec = (esr >> 4) & 7;
          auto boff = (esr >> 2) & 1;
          auto bpvf = (esr >> 1) & 1;
          auto bwgf = (esr >> 0) & 1;
          auto rerr = can_rderror(can);
          auto terr = can_tderror(can);

          logger.Info("  can1");
          logger.Info("    REC = %d, TEC = %d LEC = %d", rec, tec, lec);
          logger.Info("    Bus Off?: %d, Passive?: %d, Warning?: %d", boff,
                      bpvf, bwgf);
        }
        {
          auto can = actuators->can2.get_can_instance()->get_can();
          auto esr = can->CanHandle.Instance->ESR;
          auto rec = (esr >> 24) & 0xff;
          auto tec = (esr >> 16) & 0xff;
          auto lec = (esr >> 4) & 7;
          auto boff = (esr >> 2) & 1;
          auto bpvf = (esr >> 1) & 1;
          auto bwgf = (esr >> 0) & 1;
          auto rerr = can_rderror(can);
          auto terr = can_tderror(can);

          logger.Info("  can2");
          logger.Info("    Rer = %d, Ter = %d", rerr, terr);
          logger.Info("    REC = %d, TEC = %d LEC = %d", rec, tec, lec);
          logger.Info("    Bus Off?: %d, Passive?: %d, Warning?: %d", boff,
                      bpvf, bwgf);
        }
      } else {
        ctrl.puropo.print_debug();
        printf("\n");
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