#pragma once

#include <robotics/controller/action.hpp>
#include <robotics/controller/boolean.hpp>
#include <robotics/controller/float.hpp>
#include <robotics/controller/joystick.hpp>
#include <robotics/filter/angled_motor.hpp>
#include <nhk2024b/types.hpp>

namespace nhk2024b {
class RefrigeController {
 public:
  // どういうノードをコントローラーから生やすか
  // 四輪駆動
  // ↑controller::の形にしない！！
  class Refrige {
    template <typename T>
    using Node = robotics::Node<T>;

   public:
    Node<bool> ctr_collector;       // 回収機構多分ボタン
    Node<bool> ctr_locke;           // ロック解除の際のボタン
    Node<bool> ctr_brake;  // ブレーキのボタン
    Node<JoyStick2D> ctr_move;
    //↑コントロール側のノード


    // 出力 Node (モーターなど)
    Node<float> out_motor_1;
    Node<float> out_motor_2;
    Node<float> out_motor_3;
    Node<float> out_motor_4;
    Node<float> out_brake;      // ブレーキ
    Node<float> out_lock;       // ロック解除->戻す必要あり
    Node<float> out_collector;  // 回収機構ー＞戻さなくてもよさそう

    void LinkController() {
      ctr_move.SetChangeCallback([this](robotics::JoyStick2D stick) {

      });

      ctr_button.SetChangeCallback(
          [this](bool btn) {  // ボタン押している間にロック解除と発射（motorが回りつ図ける）
            if (btn == true) {
              lock.SetValue(0.2);
            } else {
              lock.SetValue(0);
            }
          });

      ctr_collector;
    }
  }
  }; 
  } // namespace nhk2024b