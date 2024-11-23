#pragma once

#include <cstdint>
#include <robotics/platform/panic.hpp>

namespace robobus::types {
/**
 * @class P2PPipeID
 * @brief P2Pパイプ識別子
 * @details P2P Pipe ID (14bit) の Value Object
 */
class P2PPipeID {
  /// @brief 14bit の Pipe ID
  uint16_t id_;

 public:
  /**
   * @brief コンストラクタ
   * @param id 14bit Pipe ID
   *
   * @exception std::invalid_argument PipeID must be less than 0x4000 (14
   * bit)
   */
  explicit P2PPipeID(uint16_t id) : id_(id) {
    if (0x4000 < id_) {
      robotics::system::panic("P2PPipeID must be less than 0x4000 (14 bit)");
    }
  }

  /// @brief 符号なし 32bit で P2P Pipe ID を取得
  uint16_t GetP2PPipeID() const { return id_; }
};
}  // namespace robobus::types
