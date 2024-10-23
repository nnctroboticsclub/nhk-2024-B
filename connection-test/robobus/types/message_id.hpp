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
 * @brief メッセージ ID (20bit) の Value Object
 */
class MessageID {
  uint32_t id_;

 public:
  /**
   * @brief コンストラクタ
   * @param id 20bit Message ID
   * @exception std::invalid_argument MessageID must be less than 0x100000 (20
   * bit)
   */
  explicit MessageID(uint32_t id) : id_(id) {
    if (0x100000 < id_) {
      robotics::system::panic("MessageID must be less than 0x100000 (20 bit)");
    }
  }

  /// @brief Control Transfer の Message ID を生成
  static MessageID CreateControlTransfer(DeviceID sender_device_id,
                                         DataCtrlMarker data_ctrl_marker) {
    return MessageID((0x0 << 16) | (sender_device_id.GetDeviceID() << 8) |
                     static_cast<uint32_t>(data_ctrl_marker));
  }

  /// @brief Message ID を取得
  uint32_t GetMsgID() const { return id_; }

  /// @brief メッセージの種類を取得
  MessageType GetMessageType() const {
    auto kind = (id_ & 0x70000) >> 16;  // 0 ~ 7
    switch (kind) {
      case 0:
        return MessageType::kControl;
      case 1:
        return MessageType::kRawP2P;
      case 2:
        return MessageType::kP2P;
      case 3:
        return MessageType::kMulticast;
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

  bool operator==(MessageID const &rhs) const { return id_ == rhs.id_; }
};
}  // namespace robobus::types
