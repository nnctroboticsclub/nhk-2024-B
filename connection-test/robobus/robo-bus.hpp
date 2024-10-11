#pragma once

#include <cstddef>

#include <chrono>
#include <optional>

#include <robotics/thread/thread.hpp>
#include <robotics/network/stream.hpp>
#include <robotics/network/can_base.hpp>
#include <robotics/network/simple_can.hpp>
#include <robotics/platform/panic.hpp>

#include "types/device_id.hpp"
#include "types/message_id.hpp"

#include "platform.hpp"

namespace robobus::robobus {
/// @internal
/// @brief 雑用ロガー
robotics::logger::Logger chore_logger_{"RoboBus", "robo-bus.nw"};

/// @brief バイト列をやり取りするストリーム
using ByteStream = robotics::network::Stream<uint8_t>;

class ControlStream : public ByteStream {
 public:
};

class RoboBus {
  static inline robotics::logger::Logger logger_{"RoboBus", "robo-bus.nw"};
  std::unique_ptr<robotics::network::CANBase> upper_stream_;
  DeviceID self_device_id_;
  bool is_motherboard_;

  void ProcessControlMessage(types::MessageID id,
                             std::vector<uint8_t> const &_data) const {
    auto data_ctrl_marker = id.GetDataCtrlMarker();
    auto sender_device_id = id.GetSenderDeviceID();
    if (!data_ctrl_marker || !sender_device_id) {
      logger_.Error("Invalid control message");
      return;
    }

    logger_.Info("Control message from %d", sender_device_id->GetDeviceID());
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
    upper_stream_->OnRx([this](uint32_t id, std::vector<uint8_t> const &data) {
      ProcessMessage(id, data);
    });
  }
};
}  // namespace robobus::robobus

namespace robobus {
using robobus::RoboBus;
using ::robobus::types::DeviceID;

class RoboBusTest {
 public:
  [[noreturn]]
  void Main() const {
    auto mode = platform::GetMode();

    DeviceID device_id{0};
    bool is_motherboard = false;
    switch (mode) {
      case platform::Mode::kDevice1:
        device_id = DeviceID(0);
        is_motherboard = true;
        break;
      case platform::Mode::kDevice2:
        device_id = DeviceID(1);
        is_motherboard = false;
        break;
    }

    auto simple_can =
        std::make_unique<robotics::network::SimpleCAN>(PB_8, PB_9, 1E6);
    auto can = static_cast<std::unique_ptr<robotics::network::CANBase>>(
        std::move(simple_can));

    auto robobus = std::make_unique<robobus::RoboBus>(
        DeviceID(platform::GetSelfAddress()), is_motherboard, std::move(can));

    using namespace std::chrono_literals;

    while (true) {
      robotics::system::SleepFor(100s);
    }
  }
};
}  // namespace robobus