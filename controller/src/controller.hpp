#include <mbed.h>

#include <robotics/node/node.hpp>

namespace nhk2024b {
class ControllerReader {
  template <typename T>
  using Node = robotics::Node<T>;

  Thread *thread;
  // ↓ここもいじる
  mbed::AnalogIn value_1_in;

 public:
  // ここ以下をいじる
  Node<float> value_1;

  ControllerReader() : value_1_in(PA_3) {
    thread = new Thread();
    thread->start([this]() { ControllerThread(); });
  }

  void ControllerThread() {
    while (1) {
      // ここをいじる
      value_1.SetValue(value_1_in.read());
    }
  }
};
}  // namespace nhk2024b