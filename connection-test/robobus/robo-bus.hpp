#pragma once

#include <cstddef>

#include <chrono>
#include <optional>

#include <robotics/thread/thread.hpp>
#include <robotics/network/stream.hpp>
#include <robotics/network/can_base.hpp>
#include <robotics/network/simple_can.hpp>
#include <robotics/platform/panic.hpp>
#include <rd16.hpp>

#include "types/device_id.hpp"
#include "types/message_id.hpp"

#include "../platform.hpp"

namespace robobus::robobus {
/// @internal
/// @brief 雑用ロガー
robotics::logger::Logger chore_logger_{"robo-bus.nw", "RoboBus"};

/// @brief バイト列をやり取りするストリーム
using ByteStream = robotics::network::Stream<uint8_t>;

using robotics::logger::core::Level;
using types::DeviceID;

namespace control_stream {
using robotics::network::RD16;
struct ControlStreamControlData {
  uint16_t seq;
  uint16_t remote_seq;

  uint16_t chunk_rd16;
  uint16_t whole_rd16;
};

struct ControlStream {
  ControlStreamControlData tx_ctrl;
  ControlStreamControlData rx_ctrl;
  RD16 rx_whole_rd16;
  RD16 tx_whole_rd16;

  void FeedTx() {
    RD16 tx_chunk_rd16;
    tx_chunk_rd16 << tx_ctrl.seq;
  }
};
}  // namespace control_stream

class RoboBus {
  static inline robotics::logger::Logger logger_{"robo-bus.nw", "RoboBus"};
  std::unique_ptr<robotics::network::CANBase> upper_stream_;
  types::DeviceID self_device_id_;
  bool is_motherboard_;

  void ProcessControlMessage(types::MessageID id,
                             std::vector<uint8_t> const &data) const {
    auto data_ctrl_marker = id.GetDataCtrlMarker();
    auto sender_device_id = id.GetSenderDeviceID();
    if (!data_ctrl_marker || !sender_device_id) {
      logger_.Error("Invalid control message");
      return;
    }

    // Accept with motherboard? or id=self_device_id
    if (!is_motherboard_ &&
        *sender_device_id != self_device_id_.GetDeviceID()) {
      return;
    }

    this->logger_.Debug("");
    this->logger_.Debug("Received message: %08X", id);
    this->logger_.Hex(Level::kDebug, data.data(), data.size());

    logger_.Info("Control message between %d (marker: %d)",
                 sender_device_id->GetDeviceID(),
                 static_cast<int>(*data_ctrl_marker));
  }
  void ProcessRawP2PMessage(
      [[maybe_unused]] types::MessageID _id,
      [[maybe_unused]] std::vector<uint8_t> const &_data) const {
    logger_.Error("RawP2P message is not supported");
  }
  void ProcessP2PMessage(
      [[maybe_unused]] types::MessageID _id,
      [[maybe_unused]] std::vector<uint8_t> const &_data) const {
    logger_.Error("P2P message is not supported");
  }
  void ProcessMulticastMessage(
      [[maybe_unused]] types::MessageID _id,
      [[maybe_unused]] std::vector<uint8_t> const &_data) const {
    logger_.Error("Multicast message is not supported");
  }

  void ProcessMessage(uint32_t id, std::vector<uint8_t> const &data) const {
    auto msg_id = types::MessageID(id);

    switch (msg_id.GetMessageType()) {
      case types::MessageType::kControl:
        ProcessControlMessage(msg_id, data);
        break;
      case types::MessageType::kRawP2P:
        ProcessRawP2PMessage(msg_id, data);
        break;
      case types::MessageType::kP2P:
        ProcessP2PMessage(msg_id, data);
        break;
      case types::MessageType::kMulticast:
        ProcessMulticastMessage(msg_id, data);
        break;
      default:
        robotics::system::panic("Invalid message type, this should not happen");
        break;
    }
  }

 public:
  explicit RoboBus(DeviceID device_id, bool is_motherboard,
                   std::unique_ptr<robotics::network::CANBase> upper_stream)
      : upper_stream_(std::move(upper_stream)),
        self_device_id_(device_id),
        is_motherboard_(is_motherboard) {
    logger_.Info("RoboBus initialized");
    upper_stream_->OnRx([this](uint32_t id, std::vector<uint8_t> const &data) {
      ProcessMessage(id, data);
    });

    upper_stream_->Init();
  }

  void Test(DeviceID remote_device_id) {
    DeviceID cpipe_dev_id = DeviceID(1);

    types::DataCtrlMarker data_ctrl_marker =
        is_motherboard_ ? types::DataCtrlMarker::kServerCtrl
                        : types::DataCtrlMarker::kClientCtrl;

    auto msg_id =
        types::MessageID::CreateControlTransfer(cpipe_dev_id, data_ctrl_marker);

    std::vector<uint8_t> data{0x00, 0x01, 0x00, 0x00, 0x00, 0x00};

    upper_stream_->Send(msg_id.GetMsgID(), data);
  }
};
}  // namespace robobus::robobus
