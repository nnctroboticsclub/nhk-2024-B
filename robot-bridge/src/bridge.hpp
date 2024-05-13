#pragma once

#include <robotics/controller/action.hpp>
#include <robotics/controller/float.hpp>
#include <robotics/filter/inc_angled_motor.hpp>

namespace nhk2024b {
class BridgeController {
 public:
  struct Config {
    // コントローラーの ID リスト
    int rotate_id;
    int button_id;
  };

  // どういうノードをコントローラーから生やすか
  controller::Action rotate;
  controller::Action button;

  // const はコンストラクタないで変更がないことを明記/強制
  BridgeController(const Config &config)
      : rotate(config.rotate_id), button(config.button_id) {}
};

class Bridge {
  template <typename T>
  using Node = robotics::Node<T>;

  const std::chrono::duration<float> revolver_reversing_time = 30ms;

  enum class LoadState { Start_rote };
  LoadState state;

  // コントローラー Node をまとめて持つ
  BridgeController ctrl;

 public:
  // 出力 Node (モーターなど)
  Node<float> out_a;
  using IncAngledMotor = robotics::filter::IncAngledMotor<float>;
  IncAngledMotor lock;

  Bridge(BridgeController::Config &ctrl_config) : ctrl(ctrl_config) {}

  void LinkController() {
    ctrl.rotate.Link(out_a);
    ctrl.button.OnFire([this]() {  // 押された時の処理
      lock.AddAngle(180);
    });

    ctrl.rotate.OnFire([this]() {  // 押された時の処理
      switch (state) {
        case LoadState::kInReversing:
          if (timer.elapsed_time() > revolver_reversing_time) {
            // revolver_reversing_timeは定数
            // timer.elapsed()は関数
            SetLoadState(LoadAction::kStopCounterRotation);
          }
      }
    });
  }

  void Update(float dt) { lock.Update(dt); }
};
}  // namespace nhk2024b