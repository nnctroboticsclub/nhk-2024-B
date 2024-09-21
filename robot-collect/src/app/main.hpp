#include <cinttypes>
// #include <mbed.h>

// #include "identify.h"
// #include "app.hpp"

// #include <mbed-robotics/simple_can.hpp>
#include <puropo.h>

#include <mbed-robotics/uart_stream.hpp>
#include <nhk2024b/fep_ps4_con.hpp>
#include <robotics/logger/logger.hpp>
#include <robotics/network/fep/fep_driver.hpp>
#include <robotics/platform/dout.hpp>

#include "app.hpp"
#include "collect.hpp"

robotics::logger::Logger logger{"   app   ", "app"};

void InitFEP() {
  robotics::network::UARTStream uart{PC_6, PC_7, 115200};
  robotics::driver::Dout rst{PC_9};
  robotics::driver::Dout ini{PC_8};
  robotics::network::fep::FEPDriver fep_drv{uart, rst, ini};

  robotics::logger::SuppressLogger("rxp.fep.nw");
  robotics::logger::SuppressLogger("st.fep.nw");
  robotics::logger::SuppressLogger("sr.fep.nw");

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

template <typename T>
using Node = robotics::node::Node<T>;

using robotics::types::JoyStick2D;

class PuropoController {
  Puropo puropo;

 public:
  Node<JoyStick2D> stick1;
  Node<JoyStick2D> stick2;
  Node<bool> button1;
  Node<bool> button2;
  Node<bool> button3;
  Node<bool> button4;
  Node<bool> button5;

  // コンストラクタ→初期化
  PuropoController(PinName tx, PinName rx) : puropo(tx, rx) { puropo.start(); }

  // 毎ティック実行される関数
  void Tick() {
    // プロポの値を Node に格納
    // <xxx>.SetValue(<value>);
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
  }
};

class Test {
  using PS4Con = nhk2024b::ps4_con::PS4Con;
  using Actuators = nhk2024b::robot3::Actuators;

  InterruptIn hard_emc_gpio{PA_15, PinMode::PullDown};

  Actuators actuators{(Actuators::Config){
      // M4
      .move_motor_fin = PA_9,
      .move_motor_rin = PA_8,

      // M3
      .arm_elevation_motor_fin = PB_9,
      .arm_elevation_motor_rin = PB_8,

      .arm_expansion_motor_fin = PB_10,
      .arm_expansion_motor_rin = PB_2,
  }};

  PuropoController puropo{PC_6, PC_7};

  nhk2024b::robot3::Robot robot{};

  bool ctrl_emc = false;
  bool hard_emc = false;

  void UpdateEMC() {
    bool emc_out = ctrl_emc & hard_emc;

    logger.Info("(emc_ctrl = %d) & (hard_emc = %d) -> (emc_out = %d)", ctrl_emc,
                hard_emc, emc_out);

    robot.emc_state.SetValue(emc_out);
  }

 public:
  void Init() {
    logger.Info("Init");
    puropo.stick1 >> robot.ctrl_stick_move;
    puropo.button1 >> robot.ctrl_button_arm_open;
    puropo.button2 >> robot.ctrl_button_arm_close;
    puropo.button3 >> robot.ctrl_button_arm_up;
    puropo.button4 >> robot.ctrl_button_arm_down;

    puropo.button5.SetChangeCallback([this](bool btn) {
      ctrl_emc = btn ? 0 : 1;  // pressed = 0 (stop actuators)
      UpdateEMC();
    });

    robot.out_move >> actuators.move_motor;
    robot.out_arm_elevation >> actuators.arm_elevation_motor;
    robot.LinkController();

    puropo.button5.SetValue(false);

    hard_emc_gpio.fall([this]() {
      this->hard_emc = 0;
      this->UpdateEMC();
    });
    hard_emc_gpio.rise([this]() {
      this->hard_emc = 1;
      this->UpdateEMC();
    });
    this->hard_emc = hard_emc_gpio.read();
    this->UpdateEMC();

    logger.Info("Init done");
  }

  void Main() {
    logger.Info("Main loop");

    Timer timer;
    timer.reset();
    timer.start();

    float previous = 0;

    int i = 0;
    while (1) {
      if (i % 1000 == 0) logger.Info("Update");
      puropo.Tick();

      if (i % 200 == 0) {
        logger.Info("Report");
        logger.Info("  Stick: %f, %f; %f, %f",    //
                    puropo.stick1.GetValue()[0],  //
                    puropo.stick1.GetValue()[1],  //
                    puropo.stick2.GetValue()[0],  //
                    puropo.stick2.GetValue()[1]   //
        );
        logger.Info("  output: %f %f %f",                      //
                    actuators.move_motor.GetValue(),           //
                    actuators.arm_elevation_motor.GetValue(),  //
                    actuators.arm_expansion_motor.GetValue()   //
        );
        logger.Info("  emc: %d", robot.emc_state.GetValue());
      }
      i += 1;
      ThisThread::sleep_for(1ms);
    }
  }
};

int main_0() {
  Thread thread{osPriorityNormal, 8192, nullptr, "Main"};
  thread.start([]() {
    auto test = new Test();

    test->Init();
    test->Main();
  });

  while (1) {
    ThisThread::sleep_for(100s);
  }

  return 0;
}

int main_1() { return 0; }

int main_2() { return 0; }

int main_3() { return 0; }

int main_pro() {
  /* App::Config config{        //
                     .com =  //
                     {
                         .can =
                             {
                                 .id = CAN_ID,
                                 .freqency = (int)1E6,
                                 .rx = PB_8,
                                 .tx = PB_9,
                             },
                         .driving_can =
                             {
                                 .rx = PB_5, // PB_5,
                                 .tx = PB_6, // PB_6,
                             },
                         .i2c =
                             {
                                 .sda = PC_9,
                                 .scl = PA_8,
                             },

                         .value_store_config = {},
                     },
                     .can1_debug = false};


App app(config);
  app.Init();

  while (1) {
    ThisThread::sleep_for(100s);
  }

  return 0; */
}

void main_puropo_test() {  // プロポ実験
  auto puropo = Puropo{PC_6, PC_7};
  puropo.start();
  printf("\x1b[2J");
  while (1) {
    printf("\x1b[0;0H");
    printf("is_ok: %s\n", puropo.is_ok() ? "OK" : "NG");
    printf("ch1: %6.4lf, %6.4lf, %6.4lf, %6.4lf\n",  //
           puropo.get(1), puropo.get(2), puropo.get(3), puropo.get(4));
    printf("ch5: %6.4lf, %6.4lf, %6.4lf, %6.4lf\n",  //
           puropo.get(5), puropo.get(6), puropo.get(7), puropo.get(8));
    printf("ch9: %6.4lf, %6.4lf, %6.4lf, %6.4lf\n",  //
           puropo.get(9), puropo.get(10), puropo.get(11), puropo.get(12));
    printf("ch13: %6.4lf, %6.4lf, %6.4lf, %6.4lf\n",  //
           puropo.get(13), puropo.get(14), puropo.get(15), puropo.get(16));
    printf("\n");
    ThisThread::sleep_for(10ms);
  }
}

int main_switch() {
  printf("main() started\n");
  printf("Build: " __DATE__ " - " __TIME__ "\n");

  robotics::logger::Init();

  main_0();
  return 0;
}