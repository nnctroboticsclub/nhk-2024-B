#pragma once

#include <cstddef>

#include <chrono>
#include <optional>

#include <robotics/thread/thread.hpp>
#include <robotics/network/stream.hpp>
#include <robotics/network/can_base.hpp>
#include <robotics/network/simple_can.hpp>
#include <robotics/platform/panic.hpp>
#include <robotics/timer/timer.hpp>
#include <robotics/node/node.hpp>
#include <rd16.hpp>

#include "types/device_id.hpp"
#include "types/message_id.hpp"

#include "internal/signal.hpp"
#include "internal/multi_updatable.hpp"

namespace robobus::robobus {
/// @internal
/// @brief 雑用ロガー
robotics::logger::Logger logger{"robo-bus.nw", " RoboBus "};

/// @brief バイト列をやり取りするストリーム
using ByteStream = robotics::network::Stream<uint8_t>;

using robotics::logger::core::Level;
using types::DeviceID;

/* class RoboBus {
  static inline robotics::logger::Logger logger_{"robo-bus.nw", "RoboBus"};

 public:  // TODO(syoch): Make this variable private after testing
  std::shared_ptr<robotics::network::CANBase> upper_stream_;

 private:
  types::DeviceID self_device_id_;
  bool is_motherboard_;

  void ProcessControlMessage(types::MessageID id,
                             std::vector<uint8_t> const &data) const {
    auto data_ctrl_marker = id.GetDataCtrlMarker();
    auto device_id = id.GetSenderDeviceID();
    if (!data_ctrl_marker || !device_id) {
      logger_.Error("Invalid control message");
      return;
    }

    // Accept with motherboard? or id=self_device_id
    if (!is_motherboard_ && *device_id != self_device_id_) {
      return;
    }

    this->logger_.Debug("");
    this->logger_.Debug("Received message: %08X", id);
    this->logger_.Hex(Level::kDebug, data.data(), data.size());

    logger_.Debug("Control message between %d (marker: %d)",
                  device_id->GetDeviceID(),
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
  RoboBus(DeviceID device_id, bool is_motherboard,
          std::shared_ptr<robotics::network::CANBase> upper_stream)
      : upper_stream_(std::move(upper_stream)),
        self_device_id_(device_id),
        is_motherboard_(is_motherboard) {
    logger_.Info("RoboBus initialized");
    // upper_stream_->OnRx([this](uint32_t id, std::vector<uint8_t> const &data)
    // { ProcessMessage(id, data);
    // });

    upper_stream_->Init();
  }

  void DebugCAN() {
    upper_stream_->OnTx([this](uint32_t id, std::vector<uint8_t> const &data) {
      std::stringstream ss;
      for (auto const &d : data) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)d << " ";
      }
      logger_.Info("");
      logger_.Info("[TX] %08X; %s", id, ss.str().c_str());
    });

    upper_stream_->OnRx([this](uint32_t id, std::vector<uint8_t> const &data) {
      std::stringstream ss;
      for (auto const &d : data) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)d << " ";
      }

      logger_.Info("");
      logger_.Info("[RX] %08X; %s", id, ss.str().c_str());
    });
  }
}; */
}  // namespace robobus::robobus
