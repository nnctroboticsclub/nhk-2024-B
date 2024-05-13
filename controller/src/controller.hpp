#include <mbed.h>

#include <robotics/node/node.hpp>

namespace nhk2024b {
class ControllerReader {
  template <typename T>
  using Node = robotics::Node<T>;
  // 仕事の概要:コントローラーから受け取った値を変換して更新（次の層に渡す）
  Thread *thread;
  // ↓ここもいじる
  // mbed::AnalogIn value_1_in;//mbed::AnalogInがたのメンバーであるvalue_1_in
  // 今必要な機能として挙げられるのは緊急停止,コントローラー切り替えはbutton　
  mbed::DigitalIn change_target_in;  // コントローラーの切り替え
  mbed::DigitalIn emc_in;            // 緊急停止

 public:
  // ここ以下をいじる
  // Node<float> value_1;//次の層に本当に渡されるやつ
  Node<bool> change_target;
  Node<bool> emc;

  /*ControllerReader() : value_1_in(PA_3)
  {//PA_3は仮で初期化のピンとしているだけで変わるかも thread = new
  Thread();//ポインタの初期化 thread->start([this]() { ControllerThread();
  });//threadのメンバーのstart([])...をよびだす
    //キャプチャリスト,classのなかのthis,無名関数
  }*/

  ControllerReader() : change_target_in(PA_3), emc_in(PA_4) {
    thread = new Thread();  // ポインタの初期化
    thread->start([this]() { ControllerThread(); });
  }

  void ControllerThread() {
    while (1) {
      // ここをいじる
      // value_1.SetValue(value_1_in.read());//value_1をvalue_1_inに更新
      change_target.SetValue(change_target_in.read());
      emc.SetValue(emc_in.read());
    }
  }
};
}  // namespace nhk2024b