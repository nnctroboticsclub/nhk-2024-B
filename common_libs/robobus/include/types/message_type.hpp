#pragma once

namespace robobus::types {
/**
 * @enum MessageType
 * @brief メッセージの種類を表す列挙型
 * @details メッセージ ID の 18-16 bit によって決定される
 */
enum class MessageType {
  /// デバイス間通信の制御あり転送
  kP2P = 0,
  /// マルチキャスト転送はデバイス間通信の制御なし転送
  kMulticast = 1,
  /// デバイス間での制御なし転送
  kRawP2P = 2,
  /// マザボとデバイスとの制御あり通信
  kControl = 3,
};
}  // namespace robobus::types
