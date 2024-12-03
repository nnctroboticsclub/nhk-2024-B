#pragma once

#include <cstdint>

namespace robobus::types {
/**
 * @class MulticastSessionID
 * @brief Multicast のセッション識別子
 * @details Multicast Session ID (8bit) の Value Object
 */
class MulticastSessionID {
  /// @brief 8bit の MulticastSession ID
  uint8_t id_;

 public:
  /**
   * @brief コンストラクタ
   * @param id 8bit Multicast Session ID
   */
  explicit MulticastSessionID(uint8_t id) : id_(id) {}

  /// @brief 符号なし 32bit で Multicast Session ID を取得
  uint8_t GetMulticastSessionID() const { return id_; }
};
}  // namespace robobus::types
