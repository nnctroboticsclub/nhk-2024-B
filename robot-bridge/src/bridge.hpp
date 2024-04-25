#pragma once

#include <robotics/controller/float.hpp>

namespace nhk2024b {
class ExampleController {
 public:
  struct Config {
    // コントローラーの ID リスト
    int rotate_id;
    int button_id;
  };

  // どういうノードをコントローラーから生やすか
  controller::Float rotate;
  controller::Float button;
  
  // const はコンストラクタないで変更がないことを明記/強制
  ExampleController(const Config& config):
    rotate(config.rotate_id),
    button(config.button_id)
  {
  }
};

class Example {
  template<typename T>
  using Node = robotics::Node<T>;

  // コントローラー Node をまとめて持つ
  ExampleController ctrl;

 public:
  // 出力 Node (モーターなど)
  Node<float> out_a;

  Example(ExampleController::Config &ctrl_config): ctrl(ctrl_config) {
  }

  void LinkController() {
    ctrl.rotate.Link(out_a);
  }
};
} // namespace nhk2024b