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
  Node<bool> ctrl_collector;  //３状態間を遷移させる 回収機構
  Node<bool> ctrl_unlock; //toggleに変更する アンロック
  Node<bool> ctrl_brake;  //toggleに変更する　ブレーキ

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
  Node<float> out_unlock;     // アンロック
  Node<float> out_collector;  // 回収機構ー＞戻さなくてもよさそう
  
  bool unlock_state = false;
  bool brake_state = false;
  int collecter_state = 0;

  AngledMotor unlock;
  float max_unlock_angle = 15.0;

  void Update(float dt) { unlock.Update(dt); }

  void LinkController() {
    ctrl_move.SetChangeCallback([this](robotics::JoyStick2D stick) {
      double setmotor[4] = {-M_PI / 4, M_PI / 4, 3 * M_PI / 4, 5 * M_PI / 4};

      out_motor1.SetValue(
          (cos(setmotor[0]) * stick[0] + sin(setmotor[0]) * stick[1]) * 0.7 *
          -1);
      out_motor2.SetValue(
          (cos(setmotor[1]) * stick[0] + sin(setmotor[1]) * stick[1]) * 0.7 *
          -1);
      out_motor3.SetValue(
          (cos(setmotor[2]) * stick[0] + sin(setmotor[2]) * stick[1]) * 0.7 *
          -1);
      out_motor4.SetValue(
          (cos(setmotor[3]) * stick[0] + sin(setmotor[3]) * stick[1]) * 0.7 *
          -1);
    });
    // ↓ボタンの作動

    ctrl_turning_right.SetChangeCallback([this](float trigger) {
      out_motor1.SetValue(-trigger);
      out_motor2.SetValue(trigger);
      out_motor3.SetValue(trigger);
      out_motor4.SetValue(-trigger);
    });

    ctrl_turning_left.SetChangeCallback([this](float trigger) {
      out_motor1.SetValue(trigger);
      out_motor2.SetValue(-trigger);
      out_motor3.SetValue(-trigger);
      out_motor4.SetValue(trigger);
    });

    ctrl_unlock.SetChangeCallback([this](bool btn) {//アンロックトグル
      unlock_state = unlock_state ^ btn;
      unlock.goal.SetValue(unlock_state ? max_unlock_angle : 0);
    });

    ctrl_collector.SetChangeCallback([this](bool btn) {//コレクタトグル 
      collecter_state = (collecter_state + btn) %3;
      if(collecter_state==0){
        out_collector.SetValue(0);
      }else if(collecter_state==1){
        out_collector.SetValue(0.5);
      }else{
        out_collector.SetValue(-0.5);
      }
    });

    ctrl_brake.SetChangeCallback([this](bool btn) {//ブレーキトグル
      brake_state = brake_state ^ btn;
      out_brake.SetValue(brake_state ? 0.40 : -0.40);
    });
  }
};
}  // namespace nhk2024b::robot1