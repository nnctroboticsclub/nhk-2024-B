#pragma once

#include <cstdint>

#include <optional>

#include <robotics/platform/panic.hpp>

#include "device_id.hpp"
#include "message_type.hpp"
#include "p2p_pipe_id.hpp"
#include "multicast_session_id.hpp"
#include "data_ctrl_marker.hpp"

namespace robobus::types {
/**
 * @class MessageID
 * @brief メッセージ ID (21bit) の Value Object
 */
class MessageID {
  uint32_t id_;

 public:
  /**
   * @brief コンストラクタ
   * @param id 21bit Message ID
   * @exception std::invalid_argument MessageID must be positive
   * @exception std::invalid_argument MessageID must be less than 0x200000 (21
   * bit)
   */
  explicit MessageID(uint32_t id) : id_(id) {
    if (0x2000 < id_) {
      robotics::system::panic("MessageID must be less than 0x200000 (21 bit)");
    }
  }

  /// @brief 21bit の Message ID を取得
  uint32_t GetMsgID() const { return id_; }

  /// @brief メッセージの種類を取得
  MessageType GetMessageType() const {
    auto kind = (id_ & 0x600000) >> 21;  // 0 ~ 3
    switch (kind) {
      case 0:
        return MessageType::kP2P;
      case 1:
        return MessageType::kMulticast;
      case 2:
        return MessageType::kRawP2P;
      case 3:
        return MessageType::kControl;
      default:
        robotics::system::panic(
            "Invalid message type, this should not happen (maybe bit shift "
            "error)");
    }
  }

  /// @brief  P2P Pipe ID を取得
  /// @return P2P Pipe ID
  std::optional<P2PPipeID> GetP2PPipeID() const {
    if (GetMessageType() == MessageType::kP2P) {
      return P2PPipeID((GetMsgID() & 0x00FFFC) >> 2);
    } else {
      return std::nullopt;
    }
  }

  /// @brief Multicast の Session ID を取得
  /// @return SessionID
  std::optional<MulticastSessionID> GetMulticastSessionID() const {
    if (GetMessageType() == MessageType::kMulticast) {
      return MulticastSessionID((GetMsgID() & 0x00FF00) >> 8);
    } else {
      return std::nullopt;
    }
  }

  /// @brief P2P/制御転送におけるデータのマーカーを取得
  /// @return データマーカー
  std::optional<DataCtrlMarker> GetDataCtrlMarker() const {
    switch (GetMessageType()) {
      case MessageType::kP2P:
      case MessageType::kControl:
        break;
      default:
        return std::nullopt;
    }

    switch (GetMsgID() & 0x000003) {
      case 0:
        return DataCtrlMarker::kServerData;
      case 1:
        return DataCtrlMarker::kServerCtrl;
      case 2:
        return DataCtrlMarker::kClientData;
      case 3:
        return DataCtrlMarker::kClientCtrl;
      default:
        return std::nullopt;
    }
  }

  /// @brief 送信元のデバイス ID を取得
  /// @return デバイス ID
  std::optional<DeviceID> GetSenderDeviceID() const {
    switch (GetMessageType()) {
      case MessageType::kMulticast:
      case MessageType::kRawP2P:
        return DeviceID((GetMsgID() & 0x0000FF) >> 0);
      case MessageType::kControl:
        return DeviceID((GetMsgID() & 0x00FF00) >> 8);
      default:
        return std::nullopt;
    }
  }

  /// @brief 受信側のデバイスのデバイス ID を取得
  /// @return デバイス ID
  std::optional<DeviceID> GetReceiverDeviceID() const {
    if (GetMessageType() == MessageType::kRawP2P) {
      return DeviceID((GetMsgID() & 0x00FF00) >> 8);
    } else {
      return std::nullopt;
    }
  }
};
}  // namespace robobus::types
