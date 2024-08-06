#pragma once

// syoch-robotics ライブラリから実数値をもらうコントローラーを持ってくる
#include <robotics/controller/action.hpp>
#include <robotics/controller/boolean.hpp>
#include <robotics/controller/float.hpp>
#include <robotics/controller/joystick.hpp>
#include <robotics/filter/angled_motor.hpp>
namespace nhk2024b {
class RobotController {
 public:
  struct Config {
    // コントローラーの ID リスト
    int stick_id;
    int buttonPositive_id;
    int buttonnegative_id;
  };

  // コントローラーから生えてるコントローラー Node リスト

  controller::JoyStick stick;
  controller::Boolean button_positive;
  controller::Boolean button_negative;

  RobotController(const Config &config)
      : stick(config.stick_id),
        button_positive(config.buttonPositive_id),
        button_negative(config.buttonnegative_id)

  {}
  bool Pass(const controller::RawPacket &packet) {
    return stick.Pass(packet) || button_negative.Pass(packet)|| button_positive.Pass(packet);
  }
};

class Robot  // ノードにデータを送るよ
{
  template <typename T>
  using Node = robotics::Node<T>;
  using joystick = robotics::JoyStick2D;

  // コントローラー定義
  RobotController ctrl;

  float delta_angle;

 public:
  // 出力のやつ (モーターなど)
  //  out_xxx の名前で定義してください
  Node<float> out_motor_left;
  Node<float> out_moter_right;
  Node<float> out_armangle;  // 出力Nodeの定義だよー

  // がんばって？？？

  // 下記リンクのやつを使っても OK
  // https://github.com/nnctroboticsclub/syoch-robotics/tree/main/libs

  Robot(RobotController::Config &ctrl_config) : ctrl(ctrl_config) {
    // 後でアームを初期位置の角度に移動させる動作をさせたい
    //  一番最初にしたい処理

    // なければ空で OK
  }

  void LinkController()  // プログラムの駆動部分だよー
  {
    ctrl.stick.SetChangeCallback([this](robotics::types::JoyStick2D stick) {
      out_moter_right.SetValue(stick[1] - stick[0]);
      out_motor_left.SetValue(stick[1] + stick[0]);
    });
    // ボタンの設定
    ctrl.button_positive.SetChangeCallback([this](bool btn) {
      if (btn == true) {
        delta_angle = 1;
      }
      if (btn == false) {
        delta_angle = 0;
      }
    });
    ctrl.button_negative.SetChangeCallback([this](bool btn) {
      if (btn == true) {
        delta_angle = -1;
      }
      if (btn == false) {
        delta_angle = 0;
      }
    });
  }

  void Update(float dt) {
    float angle = out_armangle.GetValue();
    angle += delta_angle;
    if (angle >= 180) {
      delta_angle = 0;
    }
    if (angle <= 0) {
      delta_angle = 0;
    }
    out_armangle.SetValue(angle);
  }

  // ctrl.arm.Onfire();

  // コントローラー (ctrl オブジェクト) から色々生やす
  // この例ではコントローラー rotate の値をモーターの出力にリンクしている
  // ctrl.right.Link(out_right);
  // ctrl.left.Link(out_left);

  // ctrl.arm.Onfire([this](){
  //.AddAngle()
  //})

  // ctrl.rotate >> out_motor_0;
  // と書いても OK
};
}  // namespace nhk2024b