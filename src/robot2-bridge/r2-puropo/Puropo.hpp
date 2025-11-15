#pragma once

#include "NHKPuropo.h"

#include <robotics/node/node.hpp>

class PuropoController {
 public:
  NHK_Puropo puropo;

  Node<JoyStick2D> stick1;
  Node<JoyStick2D> stick2;
  Node<bool> button1;
  Node<bool> button2;
  Node<bool> button3;
  Node<bool> button4;
  Node<bool> button5;

  Node<bool> status;

  // コンストラクタ→初期化
  PuropoController(PinName tx, PinName rx) : puropo(tx, rx) {
    puropo.setup();
    stick1.SetValue(JoyStick2D{0, 0});
    stick2.SetValue(JoyStick2D{0, 0});
    button1.SetValue(false);
    button2.SetValue(false);
    button3.SetValue(false);
    button4.SetValue(false);
    button5.SetValue(false);
    status.SetValue(false);
  }

  // 毎ティック実行される関数
  void Tick() {
    // プロポの値を Node に格納
    // <xxx>.SetValue(<value>);

    puropo.update();

    auto stick1_value = JoyStick2D{-1 * puropo.get(4), puropo.get(2)};
    auto stick2_value = JoyStick2D{-1 * puropo.get(1), puropo.get(3)};
    stick1.SetValue(stick1_value);
    stick2.SetValue(stick2_value);
    button1.SetValue((puropo.get(5) + 1) / 2);
    button2.SetValue((puropo.get(6) + 1) / 2);
    button3.SetValue((puropo.get(7) + 1) /
                     2);  // Cボタンだけ真ん中に立てられるがそれはしないこと
    button4.SetValue((puropo.get(8) + 1) / 2);
    button5.SetValue((puropo.get(10) + 1) / 2);

    status.SetValue(this->puropo.is_ok());
  }
};