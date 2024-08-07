#include <MotorController.h>
#include <ikako_m3508.h>
#include <ikarashiCAN_mk2.h>
#include <mbed.h>

#include <cinttypes>
#include <mbed-robotics/ikakorobomas_node.hpp>
#include <mbed-robotics/simple_can.hpp>
#include <robotics/platform/pwm.hpp>
#include <robotics/node/BD621x.hpp>
#include <robotics/experimental/scheduler.hpp>
#include <vector>

#include "app.hpp"
#include "identify.h"
#include <can_servo.h>

robotics::logger::Logger logger{"app", "Main  App"};

class IkakoRobomasBus {
  ikarashiCAN_mk2 &can;  // must be global lifetime
  IkakoRobomasSender sender;
  std::vector<IkakoRobomasNode *> nodes;

 public:
  IkakoRobomasBus(ikarashiCAN_mk2 &can) : can(can), sender(&can) {}

  void Write() { sender.write(); }

  IkakoRobomasNode *NewNode(int index) {
    auto node = new IkakoRobomasNode(index);
    sender.set_motors(node->GetIkakoM3508().get_motor());
    nodes.push_back(node);
    return node;
  }

  // dt = 0.001 (1ms)
  void Update() {
    for (auto node : nodes) {
      node->Update();
    }
  }

  void Tick() { sender.read(); }

  bool ReadSuccess() { return can.get_read_flag(); }
};

class IkakoRobomasTest {
 public:
  DigitalOut led{LED1};
  ikarashiCAN_mk2 can{PB_8, PB_9, 0, 1000000};

  IkakoRobomasBus test_to{can};
  IkakoRobomasNode *motor_;

  Ticker ticker;

  IkakoRobomasTest() : test_to(can), motor_(test_to.NewNode(0)) {
    can.read_start();
  }

  // tickerで1msごとに割り込み処理させる関数
  void TickISR() { test_to.Update(); }

  // 約1msごとに処理される関数。この関数の中では通信処理ができる
  void Tick() { test_to.Write(); }

  int Main() {
    led = 0;
    ticker.attach([this]() { TickISR(); }, 1ms);
    while (1) {  // ここから下が無限ループ内
      Tick();

      test_to.Tick();
      led = test_to.ReadSuccess();

      motor_->velocity.SetValue(2);
    }
  }
};

int main_0() {  // ここの下に書く
  logger.Info("IkakoRobomasTest started");

  IkakoRobomasTest *test = new IkakoRobomasTest;
  test->Main();

  return 0;
}

int main_1() {
  using namespace std::chrono_literals;
  printf("BD621x test started\n");

  auto fin = std::make_shared<robotics::driver::PWM>(PC_8);
  auto rin = std::make_shared<robotics::driver::PWM>(PC_9);

  robotics::node::BD621x bd{rin, fin};

  int i = 0;
  while (1) {
    float velocity = sin(i++ / 200.0);
    if (-0.25 < velocity && velocity < 0.25) velocity = 0;
    velocity = velocity * 0.3;

    bd.SetValue(velocity);

    ThisThread::sleep_for(1ms);
  }
  return 0;
}

int main_2() {
  ikarashiCAN_mk2 can{
      PA_11,
      PA_12,
      2,
  } can_servo servo(&ican, 2);
  return 0;
}

int main_pro() {
  logger.Info("Prod code started");
  App::Config config{        //
                     .com =  //
                     {
                         .can =
                             {
                                 .id = CAN_ID,
                                 .freqency = (int)1E6,
                                 .rx = PA_11,
                                 .tx = PA_12,
                             },
                         .driving_can =
                             {
                                 .rx = PB_5,
                                 .tx = PB_6,
                             },
                         .i2c =
                             {
                                 .sda = PB_9,
                                 .scl = PB_8,
                             },

                         .value_store_config = {},
                     },
                     .bridge_ctrl =
                         {
                             .move_id = 0,
                             .deploy_id = 1,
                             .test_unlock_inc_id = 50,
                             .test_unlock_dec_id = 51,
                         },
                     .can1_debug = false};

  App app(config);
  app.Init();

  while (1) {
    ThisThread::sleep_for(100s);
  }

  return 0;
}

int main_switch() {
  printf("main() started\n");
  printf("Build: " __DATE__ " - " __TIME__ "\n");
  robotics::logger::Init();

  main_2();

  return 0;
}
