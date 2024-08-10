#include <mbed.h>
#include <ikako_m3508.h>
#include <ikarashiCAN_mk2.h>
#include <vector>
#include <MotorController.h>

DigitalIn button(PC_0);
DigitalOut led(PB_7);
ikarashiCAN_mk2 can(PA_11, PA_12, 0, 1E6);
std::vector<IkakoM3508> m3;
IkakoRobomasSender motor(&can);
std::vector<MotorController> controller;
MotorParams *m3508;
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

// モータの事前処理。絶対に1番最初に実行するように！！（経験者は語る）
void motor_setup() {
  m3.push_back(IkakoM3508(1));
  motor.set_motors(m3[0].get_motor());
  m3508 = m3[0].get_motor_params();
  m3508->D = 0.0;
  m3508->J = 0.04;
  controller.push_back(
      MotorController(ControlType::VELOCITY, m3508, cprm.Ts, cprm.omega));
  controller[0].set_limit(-cprm.current_limit, cprm.current_limit);
  controller[0].set_pid_gain(cprm.kp, cprm.ki, cprm.kd);
  controller[0].start();
}

// MotorController関係の処理
void motor_controller_update() {
  if (m3[0].get_read_flag()) controller[0].set_response(m3[0].get_vel());
  controller[0].update();
}

// tickerで1msごとに割り込み処理させる関数
void attach_function() {
  cnt.attach_time++;
  cnt._1ms++;
  motor_controller_update();
}

// 約1msごとに処理される関数。この関数の中では通信処理ができる
void attach_1ms_function() { motor.write(); }

// メインの無限ループ
void main_update() {
  motor.read();
  led = m3[0].get_read_flag();
  mrp.speed = (int)button ? mrp.speed_ref : -mrp.speed_ref;
  controller[0].set_reference(mrp.speed);
  m3[0].set_ref(controller[0].get_output());
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
  printf("current:'%0.2f', ", m3[0].get_current());
  printf("velocity:'%0.2f %0.2f', ", m3[0].get_vel(), mrp.speed);
  printf("motor:'%d %d %d', ", motor.motor[0]->mc.current,
         motor.motor[0]->mc.data_array[0], motor.motor[0]->mc.data_array[1]);
  printf("out:'%d %d %d', ", motor.df_0x200.current[0],
         motor.df_0x200.data_array[0], motor.df_0x200.data_array[1]);
}

int main() {
  motor_setup();
  led = 1;
  ThisThread::sleep_for(1s);
  led = 0;

  timer.start();
  timer.reset();
  can.read_start();
  ticker.attach(&attach_function, 1ms);
  while (1) {
    // こうすることで1ms割り込みで通信などの処理ができる
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