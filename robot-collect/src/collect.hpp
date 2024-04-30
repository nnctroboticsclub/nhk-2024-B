#pragma once

// syoch-robotics ライブラリから実数値をもらうコントローラーを持ってくる
#include <robotics/controller/float.hpp>

namespace nhk2024b {
class RobotController {
 public:
  struct Config {
    // コントローラーの ID リスト
    int rotate_id;
    int button_id;
  };

  // コントローラーから生えてるコントローラー Node リスト
  controller::Float rotate;
  controller::Float button;
  
  RobotController(const Config& config):
    rotate(config.rotate_id),
    button(config.button_id)
  {
  }
};

class Robot {
  template<typename T>
  using Node = robotics::Node<T>;

  // コントローラー定義
  RobotController ctrl;

 public:
  // 出力のやつ (モーターなど)
  // out_xxx の名前で定義してください
  Node<float> out_motor_0;

  // 下記リンクのやつを使っても OK
  // https://github.com/nnctroboticsclub/syoch-robotics/tree/main/libs

  Robot(RobotController::Config &ctrl_config): ctrl(ctrl_config) {
    // 一番最初にしたい処理
    // なければ空で OK
  }

  void LinkController() {
    // コントローラー (ctrl オブジェクト) から色々生やす
    // この例ではコントローラー rotate の値をモーターの出力にリンクしている
    ctrl.rotate.Link(out_motor_0);

    // ctrl.rotate >> out_motor_0;
    // と書いても OK
  }
};
} // namespace nhk2024b