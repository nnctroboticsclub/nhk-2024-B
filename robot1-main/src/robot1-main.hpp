#pragma once

// #include <nhk2024b/types.hpp>
#include <math.h>

#include <robotics/filter/angled_motor.hpp>
#include <robotics/node/node.hpp>
#include <robotics/types/joystick_2d.hpp>

namespace nhk2024b::robot1 {
class Refrige {
  template <typename T>
  using Node = robotics::Node<T>;
  using AngledMotor = robotics::filter::AngledMotor<float>;

 public:
  Node<bool> ctrl_collector;  // ３状態間を遷移させる 回収機構
  Node<bool> ctrl_unlock;     // toggleに変更する アンロック
  Node<bool> ctrl_brake;      // toggleに変更する　ブレーキ
  Node<bool> ctrl_brake_back;

  Node<float> ctrl_turning_right;  // 右側に旋回
  Node<float> ctrl_turning_left;   // 左側に旋回
  Node<JoyStick2D> ctrl_move;
  // ↑コントロール側のノード

  // 出力 Node (モーターなど)
  Node<float> out_motor1;
  Node<float> out_motor2;
  Node<float> out_motor3;
  Node<float> out_motor4;

  Node<float> out_brake;      // ブレーキ
  Node<float> out_collector;  // 回収機構ー＞戻さなくてもよさそう
  Node<float> out_unlock;     // アンロック （タイマー）

  bool unlock_state = false;
  int brake_state = false;
  int collecter_state = 0;

  float unlock_stop_limit_s = 0;

  // float max_unlock_angle = 15.0;

  void Update(float dt) {
    if (unlock_stop_limit_s > 0) {
      unlock_stop_limit_s -= dt;

      if (unlock_stop_limit_s < 0) {
        logger.Info("Stop unlock motor");
        out_unlock.SetValue(0);
        unlock_stop_limit_s = 0;
      }
    }
  }

  void LinkController() {
    ctrl_move.SetChangeCallback([this](robotics::JoyStick2D stick) {
      double setmotor[4] = {-M_PI / 4, M_PI / 4, 3 * M_PI / 4, 5 * M_PI / 4};

      out_motor1.SetValue(
          (cos(setmotor[0]) * stick[0] + sin(setmotor[0]) * stick[1]) * 0.8 *
          -1);
      out_motor2.SetValue(
          (cos(setmotor[1]) * stick[0] + sin(setmotor[1]) * stick[1]) * 0.8 *
          -1);
      out_motor3.SetValue(
          (cos(setmotor[2]) * stick[0] + sin(setmotor[2]) * stick[1]) * 0.8 *
          -1);
      out_motor4.SetValue(
          (cos(setmotor[3]) * stick[0] + sin(setmotor[3]) * stick[1]) * 0.8 *
          -1);
    });
    // ↓ボタンの作動

    ctrl_turning_right.SetChangeCallback([this](float trigger) {
      out_motor1.SetValue(-trigger * 0.8);
      out_motor2.SetValue(-trigger * 0.8);
      out_motor3.SetValue(-trigger * 0.8);
      out_motor4.SetValue(-trigger * 0.8);
    });

    ctrl_turning_left.SetChangeCallback([this](float trigger) {
      out_motor1.SetValue(trigger * 0.8);
      out_motor2.SetValue(trigger * 0.8);
      out_motor3.SetValue(trigger * 0.8);
      out_motor4.SetValue(trigger * 0.8);
    });

    ctrl_unlock.SetChangeCallback([this](bool btn) {  // アンロックトグル
      out_unlock.SetValue(-0.3);
      unlock_stop_limit_s = 0.15;
    });

    ctrl_brake_back.SetChangeCallback([this](bool btn) {  // アンロックトグル
      out_brake.SetValue(btn ? -1 : 0);
    });

    ctrl_brake.SetChangeCallback([this](bool btn) {  // アンロックトグル
      out_brake.SetValue(btn ? 1 : 0);
    });

    ctrl_collector.SetChangeCallback([this](bool btn) {  // コレクタトグル
      collecter_state = (collecter_state + btn) % 3;
      if (collecter_state == 0) {
        out_collector.SetValue(0);
      } else if (collecter_state == 1) {
        out_collector.SetValue(0.7);
      } else {
        out_collector.SetValue(-0.7);
      }
    });
  }
};
}  // namespace nhk2024b::robot1