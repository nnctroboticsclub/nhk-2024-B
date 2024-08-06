#include <MotorController.h>
#include <ikako_m3508.h>
#include <ikarashiCAN_mk2.h>
#include <mbed.h>

#include <cinttypes>
#include <mbed-robotics/ikakorobomas_node.hpp>
#include <mbed-robotics/simple_can.hpp>
#include <vector>

#include "app.hpp"
#include "identify.h"

class IkakoRobomasTest {
 public:
  DigitalIn button{PC_13};
  DigitalOut led{LED1};
  ikarashiCAN_mk2 can{PB_8, PB_9, 0, 1000000};

  IkakoRobomasSender motor{&can};

  IkakoRobomasNode test_to;

  Ticker ticker;
  Timer timer;

  struct controller_param {
    float Ts = 0.001;
    float kp = 1.0;
    float ki = 0;
    float kd = 0;
    float current_limit = 5.0;
    float omega = 1 * 2 * M_PI;
  };
  controller_param cprm;

  struct counter {
    int attach_time = 0;
    int _1ms = 0;
  };
  counter cnt;

  struct motor_reference_param {
    float current = 0;
    const float current_ref = 0.005;
    float speed = 0;
    const float speed_ref = 1 * 2 * M_PI;
  };
  motor_reference_param mrp;

  struct Measure {
    int time[2] = {0, 0};
    int dt = 0;
  };
  Measure measure;

  IkakoRobomasTest() : test_to(1) {
    // モータの事前処理。絶対に1番最初に実行するように！！（経験者は語る）
    motor.set_motors(test_to.GetIkakoM3508().get_motor());
  }

  // tickerで1msごとに割り込み処理させる関数
  void attach_function() {
    cnt.attach_time++;
    cnt._1ms++;
    test_to.Update();

    /* if (m3.get_read_flag())
        controller->set_response(m3.get_vel());
    controller->update(); */
  }

  // 約1msごとに処理される関数。この関数の中では通信処理ができる
  void attach_1ms_function() { motor.write(); }

  // メインの無限ループ
  void main_update() {
    motor.read();
    led = test_to.GetReadFlag();
    mrp.speed = (int)button ? mrp.speed_ref : -mrp.speed_ref;

    test_to.velocity.SetValue(mrp.speed);

    /* controller->set_reference(mrp.speed);
    m3.set_ref(controller->get_output()); */
  }

  // printデバッグ用関数
  void print_debug() {
    printf("\n\r");
    printf("attach:'%d', ", cnt.attach_time);
    printf("cnt:'%d', ", cnt._1ms);
    printf("time:'%d', ", measure.dt);
    printf("button:'%d', ", (int)button);
    printf("send:'%d', ", can.get_send_flag());
    printf("read:'%d', ", can.get_read_flag());
    printf("current:'%0.2f', ", test_to.->get_current());
    printf("velocity:'%0.2f %0.2f', ", m3->get_vel(), mrp.speed);
    printf("motor:'%d %d %d', ", motor.motor[0]->mc.current,
           motor.motor[0]->mc.data_array[0], motor.motor[0]->mc.data_array[1]);
    printf("out:'%d %d %d', ", motor.df_0x200.current[0],
           motor.df_0x200.data_array[0], motor.df_0x200.data_array[1]);
  }

  int main() {
    led = 0;
    timer.start();
    timer.reset();
    can.read_start();
    ticker.attach([this]() { attach_function(); }, 1ms);
    while (1) {  // ここから下が無限ループ内
      // ↓こうすることで1ms割り込みで通信などの処理ができる
      if (cnt.attach_time > 0) {
        measure.dt = timer.read_ms() - measure.time[0];
        measure.time[0] = timer.read_ms();
        attach_1ms_function();
        if (!(cnt._1ms % 100)) print_debug();
        cnt.attach_time--;
      }

      main_update();
    }
  }
};

int main_0() {  // ここの下に書く
  IkakoRobomasTest *test = new IkakoRobomasTest;
  test->main();

  return 0;
}

int main_1() { return 0; }

int main_2() { return 0; }

int main_3() { return 0; }

int main_pro() {
  App::Config config{        //
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
                                 .rx = PB_5, /* PB_5, */
                                 .tx = PB_6, /* PB_6, */
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

  return 0;
}

int main_switch() {
  printf("main() started\n");
  printf("Build: " __DATE__ " - " __TIME__ "\n");

  main_0();
  return 0;
}
