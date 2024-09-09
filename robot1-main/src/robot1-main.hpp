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

 public:
  Node<bool> ctrl_collector;  // 回収機構ボタン（逆向きがもしかしたら必要かも）
  Node<bool> ctrl_unlock;  // ロック解除の際のボタン　逆向き必要
  Node<bool> ctrl_unlock_back;  // ロック解除の際のボタン　逆向き必要
  Node<bool> ctrl_brake;        // ブレーキのボタン逆向きが必要
  Node<bool> ctrl_brake_back;     // ブレーキのボタン逆向きが必要
  Node<bool> ctrl_brake_back;     // ブレーキのボタン逆向きが必要
  Node<bool> ctrl_turning_right;  // 右側に旋回
  Node<bool> ctrl_turning_left;   // 左側に旋回

  Node<JoyStick2D> ctrl_move;
  // ↑コントロール側のノード

  // 出力 Node (モーターなど)
  Node<float> out_motor1;
  Node<float> out_motor2;
  Node<float> out_motor3;
  Node<float> out_motor4;

  Node<float> out_brake;      // ブレーキ
  Node<float> out_unlock;     // ロック解除->戻す必要あり
  Node<float> out_collector;  // 回収機構ー＞戻さなくてもよさそう
  Node<float> out_turning_right;
  Node<float> out_turning_left;

  void LinkController() {
    ctrl_move.SetChangeCallback([this](robotics::JoyStick2D stick) {
      double setmotor[4] = {-M_PI / 4, M_PI / 4, 3 * M_PI / 4, 5 * M_PI / 4};

      out_motor1.SetValue((cos(setmotor[0]) * stick[0] + sin(setmotor[0]) * stick[1]) * 0.7 *-1);
      out_motor2.SetValue((cos(setmotor[1]) * stick[0] + sin(setmotor[1]) * stick[1]) * 0.7 *-1);
      out_motor3.SetValue((cos(setmotor[2]) * stick[0] + sin(setmotor[2]) * stick[1]) * 0.7 *-1);
      out_motor4.SetValue((cos(setmotor[3]) * stick[0] + sin(setmotor[3]) * stick[1]) * 0.7 *-1);
    });
    // ↓ボタンの作動
    ctrl_unlock.SetChangeCallback([this](bool btn) {  out_unlock.SetValue(btn ? -0.4 : 0); }); // ロック解除

    ctrl_unlock_back.SetChangeCallback([this](bool btn) { out_unlock.SetValue(btn ? 0.4 : 0); });

    ctrl_collector.SetChangeCallback([this](bool btn) { out_collector.SetValue(btn ? 0.5 : 0); });

    ctrl_brake.SetChangeCallback([this](bool btn) { out_brake.SetValue(btn ? -0.9 : 0);  }); // ブレーキ　ボタン押しているときだけ逆回転

    ctrl_brake_back.SetChangeCallback([this](bool btn) { out_brake.SetValue(btn ? 0.9 : 0); }); // ブレーキ逆向き

    ctrl_turning_right.SetChangeCallback([this](bool btn) { 
      out_motor1.SetValue(-0.7);
      out_motor2.SetValue(0.7);
      out_motor3.SetValue(-0.7);
      out_motor4.SetValue(0.7);
    });

    ctrl_turning_left.SetChangeCallback([this](bool btn) {
      out_motor1.SetValue(-0.7);
      out_motor2.SetValue(0.7);
      out_motor3.SetValue(-0.7);
      out_motor4.SetValue(0.7);
    });
  }
};
}  // namespace nhk2024b::robot1