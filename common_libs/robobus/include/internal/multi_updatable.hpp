#pragma once

#include <optional>

#include "signal.hpp"

namespace robobus::internal {
/**
 * @brief データの更新を管理するクラス
 * @details 更新操作を任意の型に付随することができる
 */
template <typename T>
class MultiUpdatable {
  static constexpr const float kTimeout_s = 10E-3f;  // 10ms

  bool need_update_ = false;
  float timer_ = kTimeout_s;

  std::optional<T> data_ = std::nullopt;

  internal::SignalTx<T> updated_tx;

 public:
  internal::SignalRx<T> updated;

  explicit MultiUpdatable(T const &data)
      : data_(data),
        updated_tx(std::make_shared<internal::Signal<T>>()),
        updated(updated_tx) {}

  MultiUpdatable()
      : updated_tx(std::make_shared<internal::Signal<T>>()),
        updated(updated_tx) {}

  /// @brief データに変化があったことを通知
  void Update() {
    need_update_ = true;
    timer_ = 0;

    if (data_ != std::nullopt) {
      updated_tx.Fire(*data_);
    }
  }

  /// @brief データの更新が正常に行われたことを通知
  void Reset() { need_update_ = false; }

  /// @param delta_time_s 前回の Tick() 呼び出しからの経過時間 [s]
  void Tick(float delta_time_s) {
    if (!need_update_) {
      return;
    }

    timer_ -= delta_time_s;
    if (timer_ <= 0) {
      timer_ = kTimeout_s;
      if (data_ != std::nullopt) updated_tx.Fire(*data_);
    }
  }

  std::optional<T> &GetOptional() { return data_; }

  T &operator*() { return *data_; }
  T const &operator*() const { return *data_; }

  T *operator->() { return &*data_; }
  T const *operator->() const { return &*data_; }
};
}  // namespace robobus::internal
