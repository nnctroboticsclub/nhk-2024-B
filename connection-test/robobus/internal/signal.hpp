#pragma once

#include <vector>
#include <functional>

namespace robobus::internal {

/// @brief Signal(クラスを超えて通知するためのもの)
/// @details Signal は Slot と呼ばれる関数を登録し、Fire
/// することで登録された関数をすべて呼び出すことができる
class Signal {
  std::vector<std::function<void()>> slots;

 public:
  /// @brief Slot を登録する
  void Connect(std::function<void()> slot) { slots.push_back(slot); }

  /// @brief 登録された Slot をすべて呼び出す (Signal が発火する)
  void Fire() {
    for (auto &slot : slots) {
      slot();
    }
  }
};
}  // namespace robobus::internal