#include <logger/generic_logger.hpp>
#include <logger/logger.hpp>
#include <nhk2024b/controller_network.hpp>
#include <nhk2024b/fep.hpp>
#include <nhk2024b/fep_ps4_con.hpp>
#include <robotics/driver/dout.hpp>
#include <robotics/network/fep/fep_driver.hpp>
#include <robotics/network/simple_can.hpp>
#include <robotics/network/uart_stream.hpp>
#include "app.hpp"
#include "bridge.hpp"

robotics::logger::Logger logger{"robot2.app", "Robot2App"};

template <typename T>
using Node = robotics::node::Node<T>;

using robotics::types::JoyStick2D;

class App {
  using Actuators = nhk2024b::robot2::Actuators;
  using Robot = nhk2024b::robot2::Robot;

  DigitalOut emc{D13};

  Actuators* actuators = new Actuators{(Actuators::Config){
      .can_1_rd = PB_5,
      .can_1_td = PB_6,
      .can_2_rd = PB_8,
      .can_2_td = PB_9,
  }};

  // nhk2024b::ControllerNetwork ctrl_net;
  // nhk2024b::robot2::Controller *ctrl;
  // PuropoController ctrl{PA_0, PA_1};
  nhk2024b::ps4_con::PS4Con ctrl{PA_0, PA_1};

  Robot robot;

  PwmOut led0{PA_6};
  PwmOut led1{PA_7};
  DigitalOut can_send_failed{PA_4};

  bool emc_ctrl = true;
  bool emc_conn = true;

  void UpdateEMC() {
    bool emc_state = emc_ctrl && emc_conn;
    emc.write(emc_state);
  }

 public:
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

    // robot.out_move_l >> actuators->move_l.velocity;
    // robot.out_move_r >> actuators->move_r.velocity;
    // robot.out_deploy >> actuators->deploy.velocity;
    // actuators->move_l.velocity.SetValue(0);
    // actuators->move_r.velocity.SetValue(0);

    // robot.out_bridge_unlock_duty.SetChangeCallback([this](float duty) {
    //   actuators->servo_0.SetValue(102.0f + 85.0f * duty);
    //   actuators->servo_1.SetValue(177.8f - 85.0f * duty);
    // });
    // actuators->servo_0.SetValue(102.0f);
    // actuators->servo_1.SetValue(177.8f);

    //robot.out_unlock_duty.SetChangeCallback([this](float duty) {
    //  actuators->servo_2.SetValue(96.0f - 96.0f * duty);
    //  actuators->servo_3.SetValue(96.0f - 96.0f * duty);
    //});
    //actuators->servo_2.SetValue(96.0f);
    //actuators->servo_3.SetValue(96.0f);

    logger.Info("Init - LED");
    // actuators->move_l.velocity.SetChangeCallback(
    //     [this](float v) { led0.write(v); });
    // actuators->move_r.velocity.SetChangeCallback(
    //     [this](float v) { led1.write(v); });

    logger.Info("Init - Link");

    ctrl.stick_left >> robot.ctrl_move;
    ctrl.button_cross >> robot.ctrl_deploy;
    ctrl.button_square >> robot.ctrl_bridge_toggle;
    ctrl.button_circle >> robot.ctrl_unlock;

    ctrl.button_options >> [this](bool btn) {
      emc_ctrl ^= btn;
      emc.write(emc_ctrl & emc_conn);
    };
    ctrl.Init();

    // actuators->move_l.factor.SetValue(0.2f);
    // actuators->move_r.factor.SetValue(0.2f);
    // ctrl.trigger_r.SetChangeCallback([this](float v) {
    //   actuators->move_l.factor.SetValue(0.2f + 0.8f * v);
    //   actuators->move_r.factor.SetValue(0.2f + 0.8f * v);
    // });

    // actuators->deploy.factor.SetValue(0.6f);

    // actuators->deploy.velocity.SetValue(0);
    // actuators->move_l.velocity.SetValue(0);
    // actuators->move_r.velocity.SetValue(0);

    robot.LinkController();

    emc.write(1);
    actuators->Init();

    logger.Info("Init - Done");
  }
  [[noreturn]]
  void Main() {
    logger.Info("Main loop");
    int i = 0;

    Timer timer;
    timer.reset();
    timer.start();

    while (true) {
      // ctrl_net.keep_alive->Update(delta_s);
      ctrl.Update();

      actuators->Tick();

      actuators->Read();
      int status_actuators_send_ = actuators->Send();
      can_send_failed = status_actuators_send_ != 0;

      if (i % 50 == 0 && true) {
        auto stick = ctrl.stick_left.GetValue();
        logger.Info("Status");
        logger.Info("  actuators_send %d", status_actuators_send_);
        // logger.Info("  ctrl.status [%d]", ctrl.status.GetValue());
        logger.Info("Report");
        logger.Info("  s %f, %f", stick[0], stick[1]);
        logger.Info("  b d(cir)%d, b(crs)%d", ctrl.button_circle.GetValue(),
                    ctrl.button_cross.GetValue());
#ifdef R2_USE_SERVO
        logger.Info("  o s %f %f | %f %f",  ///
                    actuators->servo_0.GetValue(),
                    actuators->servo_1.GetValue(),
                    actuators->servo_2.GetValue(),
                    actuators->servo_3.GetValue()  //
        );
#endif
#ifdef R2_USE_ROBOMAS
        logger.Info("    m %f %f %f", actuators->move_l.velocity.GetValue(),
                    actuators->move_r.velocity.GetValue(),
                    actuators->deploy.velocity.GetValue());
#endif

        logger.Info("Network");
        /* {
          auto can = actuators->can1.get_can_instance()->get_can();
          auto esr = can->CanHandle.Instance->ESR;
          auto rec = (esr >> 24) & 0xff;
          auto tec = (esr >> 16) & 0xff;
          auto lec = (esr >> 4) & 7;
          auto boff = (esr >> 2) & 1;
          auto bpvf = (esr >> 1) & 1;
          auto bwgf = (esr >> 0) & 1;

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
        } */
      } else {
        // ctrl.puropo.print_debug();
        // printf("\n");
      }
      i += 1;
      ThisThread::sleep_for(1ms);
    }
  }
};

int main_switch() {
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