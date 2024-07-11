#pragma once

#include <robotics/controller/action.hpp>
#include <robotics/controller/boolean.hpp>
#include <robotics/controller/float.hpp>
#include <robotics/controller/joystick.hpp>
#include <robotics/filter/angled_motor.hpp>

namespace nhk2024b {
class RefrigeController {
 public:
  struct Config {
    // コントローラーの ID リスト
    int move_id;
    int corect_id;
    int button_id;
  };

  // どういうノードをコントローラーから生やすか
  controller::Float collector;
  controller::Action button;
  controller::Float move;

  // const はコンストラクタないで変更がないことを明記/強制
  RefrigeController(const Config &config)
      : collector(config.corect_id),
        button(config.button_id),
        move(config.move_id) {}

  bool Pass(const controller::RawPacket &packet) {
    return collector.Pass(packet) || button.Pass(packet) || move.Pass(packet);
  }
};

class Refrige {
  template <typename T>
  using Node = robotics::Node<T>;

  // コントローラー Node をまとめて持つ
  RefrigeController ctrl;

 public:
  // 出力 Node (モーターなど)
  Node<float> motor_1;
  Node<float> motor_2;
  Node<float> motor_3;
  Node<float> motor_4;
  Node<float> lock;
  Node<float> collector;

  Refrige(RefrigeController::Config &ctrl_config) : ctrl(ctrl_config) {}

  void LinkController() {
    ctrl.button.SetChangeCallback([this](bool btn) {//ボタン押している間にロック解除と発射（motorが回りつ図ける）
      if (btn == true) {
        lock.SetValue(0.2);
      } else {
        lock.SetValue(0);
      }
    });
  }



};  // namespace nhk2024b