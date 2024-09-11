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
#include <nhk2024b/fep.hpp>

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

uint16_t process(float goal, float feedback) {
  float rpm_diff = (goal - feedback);

  float ampare = rpm_diff * 0.06;
  if (ampare < -2)
    ampare = -2;
  else if (2 < ampare)
    ampare = 2;

  float ratio = ampare * (1 / 20.0f);

  int16_t raw_value = ratio * 0x7FFF;
  uint16_t value = raw_value > 0 ? raw_value : 0x8000 - raw_value;

  if (raw_value < 0) {
    value |= 0x8000;
  }

  printf("--< %+04x <-> %04x >--            \n", raw_value, value);

  return value;
}

class App {
  using Actuators = nhk2024b::robot2::Actuators;
  using Robot = nhk2024b::robot2::Robot;

  DigitalOut emc{PA_15};

  /*
  using PS4Con = PseudoController;
  PS4Con ps4;
  /*/
  // using PS4Con = nhk2024b::ps4_con::PS4Con;
  // PS4Con ps4{PC_6, PC_7, 115200};
  //*/

  Actuators *actuators = new Actuators{(Actuators::Config){
      .can_1_rd = PB_5,
      .can_1_td = PB_6,
      .can_2_rd = PA_11,
      .can_2_td = PA_12,
  }};

  IkakoRobomasNode *dummy_1;
  IkakoRobomasNode *dummy_2;
  IkakoRobomasNode *dummy_3;
  // IkakoRobomasNode *dummy_4;
  // IkakoRobomasNode *dummy_5;
  // IkakoRobomasNode *dummy_6;
  // IkakoRobomasNode *dummy_7;
  // IkakoRobomasNode *dummy_8;
  // nhk2024b::common::CanServo *servo0;

  // nhk2024b::common::CanServo *servo1;

  // Robot robot;

  // robotics::network::SimpleCAN can_a{PA_11, PA_12, 1000000};

  float fb2 = 0;
  float fb3 = 0;
  int status_actuators_send_ = 0;

 public:
  App()
      : dummy_1(actuators->ikako_robomas.NewNode(2, new IkakoM3508(2))),
        dummy_2(actuators->ikako_robomas.NewNode(3, new IkakoM3508(3))),
        dummy_3(actuators->ikako_robomas.NewNode(4, new IkakoM3508(4)))
  // , dummy_4(actuators->ikako_robomas.NewNode(4))
  // , dummy_5(actuators->ikako_robomas.NewNode(5))
  // , dummy_6(actuators->ikako_robomas.NewNode(6))
  // , dummy_7(actuators->ikako_robomas.NewNode(7))
  // , dummy_8(actuators->ikako_robomas.NewNode(8))
  // , servo0(actuators->can_servo.NewNode(0))
  // , servo1(actuators->can_servo.NewNode(1))
  {}

  void Init() {
    logger.Info("Init");

    // can_a.Init();

    // ps4.stick_right >> robot.ctrl_move;
    // ps4.button_cross >> robot.ctrl_deploy;
    // ps4.button_square >> robot.ctrl_test_unlock_dec;
    // ps4.button_circle >> robot.ctrl_test_unlock_inc;

    // robot.LinkController();

    // robot.out_move_l >> dummy_2->velocity;
    // robot.out_move_r >> dummy_3->velocity;
    // robot.out_deploy >> actuators->rohm_md.in_velocity;
    // robot.out_unlock_duty.SetChangeCallback([this](float duty) {
    //   servo0->SetValue(127 + 127 * duty);
    //   servo1->SetValue(127 - 127 * duty);
    // });

    // servo0->SetValue(127);
    // servo1->SetValue(127);

    // move_l->velocity.SetValue(0);
    // move_r->velocity.SetValue(0);

    // ps4.Init();
    // ps4.Propagate();

    emc.write(1);
    /* can_a.OnRx([this](uint32_t id, std::vector<uint8_t> const& buf) {
      if (id == 0x202) {
        float rpm = buf[2] << 8 | buf[3];

        // rpm * rpm_factor / gear_ratio
        fb2 = rpm * (2.0 * M_PI / 60.0f) / (3591.0 / 187);
      } else if (id == 0x203) {
        float rpm = buf[2] << 8 | buf[3];

        // rpm * rpm_factor / gear_ratio
        fb3 = rpm * (2.0 * M_PI / 60.0f) / (3591.0 / 187);
      }
    }); */
    actuators->Init();

    logger.Info("Init - Done");
  }

  void Main() {
    logger.Info("Main loop");
    int i = 0;
    robotics::system::Timer timer;
    timer.Start();
    // printf("\x1b[2J\n");
    while (1) {
      // ps4.Update();
      actuators->Read();

      // float goal1 = 6.28 * 8;
      // float goal2 = 6.28 * 8;
      // float goal3 = 6.28 * 8;

      // uint16_t value2 = process(goal, fb2);
      // int16_t value3 = process(-goal, fb3);

      /* printf("\x1b[0;0H");
      printf("Motor2                                  \n");
      printf(
          "[rad/s] goal = %6.4lf, feedback = %6.4lf    "
          "                                 \n",
          goal, fb2);

      printf("Motor2                                  \n");
      printf(
          "[rad/s] goal = %6.4lf, feedback = %6.4lf    "
          "                                 \n",
          -goal, fb3);

      uint8_t data[] = {
          //
          0,
          0,
          (value2 >> 8) & 0xff,
          (value2 >> 0) & 0xff,
          (value3 >> 8) & 0xff,
          (value3 >> 0) & 0xff,
          0,
          0  //
      };

      for (size_t i = 0; i < 8; i++) {
        printf("%02x ", data[i]);
      }
      printf("                           \n");
      printf("                                                     \n");
      printf("                                                     \n");
      printf("                                                     \n");
      printf("                                                     \n");

      {
        auto& can = can_a;
        can.Send(0x200, std::vector<uint8_t>(data, data + 8));
      } */

      float goal1 = sin(i / 500.0 * 3.14 + 3.14 / 3.0 * 1.0) * 20;
      float goal2 = sin(i / 500.0 * 3.14 + 3.14 / 3.0 * 2.0) * 20;
      float goal3 = sin(i / 100.0 * 3.14 + 3.14 / 3.0 * 3.0) * 20;

      dummy_1->velocity.SetValue(goal1);
      dummy_2->velocity.SetValue(goal2);
      dummy_3->velocity.SetValue(goal3);

      status_actuators_send_ = actuators->Send();
      actuators->Tick();

      if (i % 50 == 0) {
        // auto stick = ps4.stick_right.GetValue();
        // logger.Info("Status");
        // logger.Info("  actuators_send %d", status_actuators_send_);
        // logger.Info("Report");
        // logger.Info("  s %f, %f", stick[0], stick[1]);
        // logger.Info("  o %f %f", servo0->GetValue(), servo1->GetValue());
        logger.Info("Output");
        logger.Info("  goal1 %6.4lf", goal1);
        logger.Info("  goal2 %6.4lf", goal2);
        logger.Info("  goal3 %6.4lf", goal3);
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

int main_switch() {
  robotics::logger::SuppressLogger("rxp.fep.nw");
  robotics::logger::SuppressLogger("st.fep.nw");
  robotics::logger::SuppressLogger("sr.fep.nw");

  // nhk2024b::InitFEP();

  return main_prod();
}