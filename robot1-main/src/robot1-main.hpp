#pragma once

//#include <nhk2024b/types.hpp>
#include <robotics/node/node.hpp>
#include <robotics/types/joystick_2d.hpp>
#include <robotics/filter/angled_motor.hpp>
#include math.h

namespace nhk2024b {
class Refrige {
  template <typename T>
  using Node = robotics::Node<T>;

 public:
  Node<bool> ctr_collector;   // 回収機構ボタン
  Node<bool> ctr_locke;       // ロック解除の際のボタン
  Node<bool> ctr_locke_back;  // ロック解除の際のボタン
  Node<bool> ctr_brake;       // ブレーキのボタン
  Node<JoyStick2D> ctr_move;
  // ↑コントロール側のノード

  // 出力 Node (モーターなど)
  Node<float> out_motor1;
  Node<float> out_motor2;
  Node<float> out_motor3;
  Node<float> out_motor4;
  Node<float> out_brake;       // ブレーキ
  Node<float> out_locke;       // ロック解除->戻す必要あり
  Node<float> out_locke_back;  // ロック解除->戻す必要あり
  Node<float> out_collector;  // 回収機構ー＞戻さなくてもよさそう

  void LinkController() {
    ctr_move.SetChangeCallback([this](robotics::JoyStick2D stick) {
      double setmotor[4]={-M_PI/4,3*M_PI/4,5*M_PI/4,7*M_PI/4};
      out_motor1.SetValue(cos(setmotor[0])*stick[1] + sin(setmotor[0])*stick[0]);
      out_motor2.SetValue(cos(setmotor[1])*stick[1] + sin(setmotor[1])*stick[0]);
      out_motor3.SetValue(cos(setmotor[2])*stick[1] + sin(setmotor[2])*stick[0]);
      out_motor4.SetValue(cos(setmotor[3])*stick[1] + sin(setmotor[3])*stick[0]);

    });
    // ↓ボタンの作動
    ctr_locke.SetChangeCallback(
        [this](bool btn) {  // ボタン押している間にロック解除と発射（motorが回りつ図ける）
          out_locke.SetValue(btn ? 0.2 : 0);
        });

    ctr_locke_back.SetChangeCallback(
        [this](bool btn) {  // 定位置に戻すためのボタン
          out_locke_back.SetValue(btn ? -0.2 : 0);
        });

    ctr_collector.SetChangeCallback(
        [this](bool btn) {  // 回収機構押しているときだけ正転
          out_collector.SetValue(btn ? 0.2 : 0);
        });

    ctr_brake.SetChangeCallback(
        [this](bool btn) {  // ブレーキ　ボタン押しているときだけ逆回転
          out_brake.SetValue(btn ? -0.2 : 0);
        });
  }
};
}  // namespace nhk2024b