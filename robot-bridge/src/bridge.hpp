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

  bool Pass(const controller::RawPacket &packet) {
    return rotate.Pass(packet) || button.Pass(packet);
  }
};

class Bridge {
  template <typename T>
  using Node = robotics::Node<T>;

  const std::chrono::duration<float> working_time = 30ms;

  enum class LoadState { In_rotate, In_stopped };
  LoadState state;

  Timer timer;

  // コントローラー Node をまとめて持つ
  BridgeController ctrl;

 public:
  // 出力 Node (モーターなど)
  Node<float> out_a;
  using IncAngledMotor = robotics::filter::IncAngledMotor<float>;
  Node<float> lock;

  Bridge(BridgeController::Config &ctrl_config) : ctrl(ctrl_config) {}

  void LinkController() {
    ctrl.rotate.Link(out_a);
    ctrl.button.OnFire([this]() {  // 押された時の処理
      state = LoadState::In_rotate;
      lock.SetValue(0.5);  //[m/s]推されたらこの速度で回して
    });
  }

  void Update(float dt) {
    switch (state) {
      case LoadState::In_rotate:

        if (timer.elapsed_time() > working_time) {
          state = LoadState::In_stopped;
          lock.SetValue(0);  //[m/s]working_time秒経過したら0m/sつまり停止する
        }
      case LoadState::In_stopped:
        break;
    }
  }
};
}  // namespace nhk2024b