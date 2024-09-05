#pragma once

//#include <nhk2024b/types.hpp>
#include <robotics/node/node.hpp>
#include <robotics/types/joystick_2d.hpp>
#include <robotics/filter/angled_motor.hpp>
#include <math.h>

namespace nhk2024b::robot1 {
class Refrige {
  template <typename T>
  using Node = robotics::Node<T>;

 public:
  Node<bool> ctrl_collector;   // 回収機構ボタン（逆向きがもしかしたら必要かも）
  Node<bool> ctrl_unlock;         // ロック解除の際のボタン　逆向き必要
  Node<bool> ctrl_unlock_back;         // ロック解除の際のボタン　逆向き必要
  Node<bool> ctrl_brake;       // ブレーキのボタン逆向きが必要
  Node<bool> ctrl_brake_back;       // ブレーキのボタン逆向きが必要
  Node<JoyStick2D> ctrl_move;
  // ↑コントロール側のノード

  // 出力 Node (モーターなど)
  Node<float> out_motor1;
  Node<float> out_motor2;
  Node<float> out_motor3;
  Node<float> out_motor4;
  Node<float> out_brake;       // ブレーキ
  Node<float> out_brake_back; //ブレーキ逆向き
  Node<float> out_unlock;       // ロック解除->戻す必要あり
  Node<float> out_unlock_back; //ロック解除を戻す
  Node<float> out_collector;  // 回収機構ー＞戻さなくてもよさそう

  void LinkController() {
    ctrl_move.SetChangeCallback([this](robotics::JoyStick2D stick) {
      double setmotor[4]={-M_PI/4,M_PI/4,3*M_PI/4,5*M_PI/4};
      out_motor1.SetValue((cos(setmotor[0])*stick[1] + sin(setmotor[0])*stick[0])*0.7);
      out_motor2.SetValue((cos(setmotor[1])*stick[1] + sin(setmotor[1])*stick[0])*0.7);
      out_motor3.SetValue((cos(setmotor[2])*stick[1] + sin(setmotor[2])*stick[0])*0.7);
      out_motor4.SetValue((cos(setmotor[3])*stick[1] + sin(setmotor[3])*stick[0])*0.7);

    });
    // ↓ボタンの作動
    ctrl_unlock.SetChangeCallback(//ロック解除
        [this](bool btn) {
          out_unlock.SetValue(btn ? -0.4 : 0);
        });

    ctrl_unlock_back.SetChangeCallback(//ロック解除
        [this](bool btn) {
          out_unlock.SetValue(btn ? 0.4 : 0);
        });

    ctrl_collector.SetChangeCallback(
        [this](bool btn) {  // 回収機構押しているときだけ正転
          out_collector.SetValue(btn ? 0.4 : 0);
        });

    ctrl_brake.SetChangeCallback(
        [this](bool btn) {  // ブレーキ　ボタン押しているときだけ逆回転
          out_brake.SetValue(btn ? -0.4 : 0);
        });
    
    ctrl_unlock.SetChangeCallback(//ブレーキ逆向き
        [this](bool btn) {
          out_unlock.SetValue(btn ? 0.4 : 0);
        });
  }
};
}  // namespace nhk2024b