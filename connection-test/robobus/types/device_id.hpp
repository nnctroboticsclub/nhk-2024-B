#pragma once

#include <cstdint>

namespace robobus::types {
/**
 * @class DeviceID
 * @brief デバイス識別子
 * @details Device ID (8bit) の Value Object
 */
class DeviceID {
  /// @brief 8bit の Device ID
  uint8_t id_;

 public:
  /**
   * @brief コンストラクタ
   * @param id 10bit Device ID
   */
  explicit DeviceID(uint8_t id) : id_(id) {}

  /// @brief 符号なし 16bit で Device ID を取得
  uint8_t GetDeviceID() const { return id_; }

  bool operator==(DeviceID const &rhs) const { return id_ == rhs.id_; }

  bool operator!=(Deviceid const &rhs) const { return id_ != rhs.id_; }
};
}  // namespace robobus::types
