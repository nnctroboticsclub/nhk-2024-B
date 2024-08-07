#pragma once

#include <robotics/controller/action.hpp>
#include <robotics/controller/float.hpp>
#include <robotics/controller/boolean.hpp>
#include <robotics/controller/joystick.hpp>
#include <robotics/filter/inc_angled_motor.hpp>

namespace nhk2024b {
class BridgeController {
 public:
  struct Config {
    // コントローラーの ID リスト
    int move_id;
    int deploy_id;
    int test_unlock_inc_id;
    int test_unlock_dec_id;
  };

  // どういうノードをコントローラーから生やすか
  controller::JoyStick move;
  controller::Boolean deploy;

  controller::Action test_unlock_inc;
  controller::Action test_unlock_dec;

  // const はコンストラクタないで変更がないことを明記/強制
  BridgeController(const Config &config)
      : move(config.move_id),
        deploy(config.deploy_id),
        test_unlock_inc(config.test_unlock_inc_id),
        test_unlock_dec(config.test_unlock_dec_id) {}
};

class Bridge {
  template <typename T>
  using Node = robotics::Node<T>;

  // 内部状態
  float unlock_duty_ = 0;
  float delta_unlock_duty_ = 0;

  // コントローラー Node をまとめて持つ
  BridgeController ctrl;

 public:
  // 出力 Node (モーターなど)
  Node<float> out_unlock;
  Node<float> out_deploy;
  Node<float> out_move_l;
  Node<float> out_move_r;

  Bridge(BridgeController::Config &ctrl_config) : ctrl(ctrl_config) {}

  void LinkController() {
    ctrl.deploy.SetChangeCallback([this](bool value) {
      if (value) {
        out_deploy.SetValue(0.2);
      } else {
        out_deploy.SetValue(0);
      }
    });
    ctrl.move.SetChangeCallback([this](robotics::types::JoyStick2D stick) {
      auto left = stick[1] + stick[0];
      auto right = stick[1] - stick[0];

      out_move_l.SetValue(left);
      out_move_r.SetValue(right);
    });
    ctrl.test_unlock_dec.OnFire([this]() {
      unlock_duty_ -= 1 / 20.0;
      out_unlock.SetValue(unlock_duty_);
    });
    ctrl.test_unlock_inc.OnFire([this]() {
      unlock_duty_ += 1 / 20.0;
      out_unlock.SetValue(unlock_duty_);
    });
  }
};
}  // namespace nhk2024b