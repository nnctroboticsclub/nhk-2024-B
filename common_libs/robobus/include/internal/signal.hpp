#pragma once

#include <utility>
#include <vector>
#include <functional>

namespace robobus::internal {

template <typename T>
class Signal;

template <typename T>
class SignalRx;

/// @brief Signal が通知するデータを送信するためのクラス
/// @tparam T Signal が通知するデータの型
template <typename T>
class SignalTx {
  std::shared_ptr<Signal<T>> signal_;

  template <typename U>
  friend class SignalRx;

 public:
  SignalTx() = delete;

  SignalTx(std::shared_ptr<Signal<T>> signal) : signal_(signal) {}

  /// @brief Signal が通知するデータを送信する
  /// @param data Signal が通知するデータ
  void Fire(T const& data) { signal_->Fire(data); }
};

/// @brief Signal が通知するデータを受け取るためのクラス
/// @tparam T Signal が通知するデータの型
template <typename T>
class SignalRx {
 public:
  std::weak_ptr<Signal<T>> signal_;

  SignalRx() = delete;

  SignalRx(std::shared_ptr<SignalTx<T>> signal_tx)
      : signal_(signal_tx->signal_) {}

  SignalRx(SignalTx<T> signal_tx) : signal_(signal_tx.signal_) {}

  SignalRx(std::shared_ptr<Signal<T>> signal) : signal_(signal) {}

  /// @brief Signal が通知するデータを受け取る
  /// @param slot Signal が通知するデータを受け取る関数
  /// @return 接続に成功した場合 true, 失敗した場合 false
  bool Connect(std::function<void(T const&)> slot) {
    if (auto signal = signal_.lock()) {
      signal->Connect(slot);
      return true;
    } else {
      return false;
    }
  }
};

/// @brief Signal(クラスを超えて通知するためのもの)
/// ちょっと高級な割り込みみたいな
/// @details Signal は Slot と呼ばれる関数を登録し、Fire
/// することで登録された関数をすべて呼び出すことができる
///
/// @tparam T Signal が通知するデータの型
template <typename T>
class Signal {
 public:
  std::vector<std::function<void(T)>> slots;

 private:
  /// @brief Slot を登録する
  void Connect(std::function<void(T const&)> slot) { slots.push_back(slot); }

  /// @brief 登録された Slot をすべて呼び出す (Signal が発火する)
  /// @param data Signal が通知するデータ
  void Fire(T const& data) {
    for (auto& slot : slots) {
      slot(data);
    }
  }

  template <typename U>
  friend class SignalTx;

  template <typename U>
  friend class SignalRx;

  template <typename U>
  friend class SignalView;

 public:
};

}  // namespace robobus::internal