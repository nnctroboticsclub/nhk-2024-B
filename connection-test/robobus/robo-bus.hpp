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
#include <rd16.hpp>

#include "types/device_id.hpp"
#include "types/message_id.hpp"
#include "internal/signal.hpp"

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
using types::DataCtrlMarker;
using types::DeviceID;
using types::MessageID;

using CANData = std::vector<uint8_t>;

/// @brief 制御ストリームの状態符号
class ControlStreamState {
  uint32_t data = 0;

 public:
  ControlStreamState(uint32_t data = 0) : data(data) {}

  void operator=(uint32_t data) { this->data = data; }

  /// @brief データを混ぜ合わせる (32bit 幅)
  /// @param code 混ぜ合わせるコード
  void MixCode(uint32_t code) { data = (data * code) ^ (data >> 16); }

  /// @brief データを取得
  uint32_t Get() const { return data; }
};

/// @brief 制御ストリームの制御データ
struct ControlStreamControlData {
  /// @brief 送信側の状態符号
  uint32_t state_transmit = 0;

  /// @brief 受信側の状態符号
  uint32_t state_receive = 0;

  /// @brief CAN データから制御データを生成 (ビッグエンディアン)
  static ControlStreamControlData FromCANData(CANData const &msg) {
    ControlStreamControlData data;
    data.state_transmit =
        (msg[0] << 24) | (msg[1] << 16) | (msg[2] << 8) | (msg[3] << 0);
    data.state_receive =
        (msg[4] << 24) | (msg[5] << 16) | (msg[6] << 8) | (msg[7] << 0);
    return data;
  }

  /// @brief 制御データから CAN データを生成
  CANData ToCANData() const {
    return {static_cast<uint8_t>((state_transmit >> 24) & 0xFF),
            static_cast<uint8_t>((state_transmit >> 16) & 0xFF),
            static_cast<uint8_t>((state_transmit >> 8) & 0xFF),
            static_cast<uint8_t>((state_transmit >> 0) & 0xFF),
            static_cast<uint8_t>((state_receive >> 24) & 0xFF),
            static_cast<uint8_t>((state_receive >> 16) & 0xFF),
            static_cast<uint8_t>((state_receive >> 8) & 0xFF),
            static_cast<uint8_t>((state_receive >> 0) & 0xFF)};
  }
};

/**
 * @brief データの更新を管理するクラス
 * @details 更新操作を任意の型に付随することができる
 */
template <typename T>
class MultiUpdatable {
  static constexpr const float kTimeout_s = 1000E-3;  // 100ms

  bool need_update_ = false;
  float timer_ = kTimeout_s;

  std::optional<T> data_ = std::nullopt;

  internal::SignalTx<T> updated_tx;

 public:
  internal::SignalRx<T> updated;

  MultiUpdatable()
      : updated_tx(std::make_shared<internal::Signal<T>>()),
        updated(updated_tx) {}

  /// @brief データに変化があったことを通知
  void Update() {
    need_update_ = true;
    timer_ = 0;

    if (data_ != std::nullopt) updated_tx.Fire(*data_);
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

/// @brief 制御ストリームの中枢実装
struct ControlStreamCore {
  //* データ周り
 public:
  /// @brief 送信側の制御データ。送信するという更新操作が存在する
  /// @note 上位レイヤでは tx_ctrl.updated シグナルを必ず処理すること
  MultiUpdatable<ControlStreamControlData> tx_ctrl;

 private:
  /// @brief 受信側の制御データ
  ControlStreamControlData rx_ctrl;

  CANData rx_data_buffer;

  /// @brief rx_available の tx 側
  internal::SignalTx<CANData> rx_available_tx;

  /// @brief tx_empty の tx 側
  internal::SignalTx<int> tx_empty_tx;

 public:
  /// @brief 送信用のデータバッファ
  /// @note 上位レイヤでは rx_data_buffer.updated シグナルを必ず処理すること
  /// (データを送信する必要がある)
  MultiUpdatable<CANData> tx_data_buffer;

  /// @brief 受信データのシグナル
  /// @note 上位レイヤでは rx_available を必ず処理すること
  internal::SignalRx<CANData> rx_available;

  /// @brief 送信データが空になったことを通知する。このシグナルに対応して
  /// FeedTxData を呼び出す
  /// @details シグナルの値はシーケンス番号
  internal::SignalRx<int> tx_empty;

  ControlStreamCore()
      : rx_available_tx(std::make_shared<internal::Signal<CANData>>()),
        tx_empty_tx(std::make_shared<internal::Signal<int>>()),
        rx_available(rx_available_tx),
        tx_empty(tx_empty_tx) {}

  //* 以下ロジック

 private:
  /// @brief rx_data_buffer, rx_ctrl から受信データを処理する
  /// @details 受信データが正常に受理された場合 rx_data_buffer
  /// を更新されたことにする
  inline void ProcessRx() {
    // Check Chunk's integrity
    if (RD16::FromData(rx_data_buffer).Get() != rx_ctrl.chunk_rd16) {
      chore_logger_.Info(
          "===> Rx; Failed to Integrity Check due to chunk rd16 mismatch");
      return;
    }

    // Check Whole data's integrity
    RD16 rx_whole_rd16_copy = rx_ctrl.whole_rd16.CopyAndAppend(rx_data_buffer);
    if (rx_whole_rd16_copy.Get() != rx_ctrl.whole_rd16.Get()) {
      chore_logger_.Info(
          "===> Rx; Failed to Integrity Check due to whole rd16 mismatch");
      return;
    }
    rx_ctrl.whole_rd16 = rx_whole_rd16_copy;

    chore_logger_.Info("===> Rx; Accepted Data");

    // Fire rx_available signal (equals to rx_data_buffer.updated)
    rx_available_tx.Fire(rx_data_buffer);

    tx_ctrl->remote_seq = rx_ctrl.seq;
    tx_ctrl.Update();
  }

  inline void ProcessTx() {
    tx_ctrl->chunk_rd16 = RD16::FromData(*tx_data_buffer).Get();
    tx_ctrl->whole_rd16 << *tx_data_buffer;

    tx_ctrl.Update();
  }

 public:
  /// @brief 送信データを更新する
  /// @param data 送信データ 8B 以下に制限する必要がある
  /// @note 送信バッファが空になる前に呼び出すとエラーになる。
  inline void FeedTxData(CANData const &data) {
    if (data.size() > 8) {
      robotics::system::panic("Invalid data size");
    }

    auto &buf = tx_data_buffer.GetOptional();
    if (!buf.has_value()) {
      buf = CANData{};
    }
    buf->resize(8);
    std::copy(data.begin(), data.end(), buf->begin());
    tx_data_buffer.Update();

    tx_ctrl->seq++;

    ProcessTx();
  }

  /// @brief 受信データを通知する
  inline void FeedRxData(CANData const &msg) {
    if (msg.size() > 8) {
      robotics::system::panic("Invalid data size");
    }

    rx_data_buffer = msg;

    ProcessRx();
  }

  inline void FeedRxCtrl(CANData const &msg) {
    if (msg.size() > 8) {
      robotics::system::panic("Invalid data size");
    }

    rx_ctrl = ControlStreamControlData::FromCANData(msg);

    if (tx_ctrl->seq == rx_ctrl.remote_seq) {
      tx_data_buffer.Reset();

      tx_empty_tx.Fire(tx_ctrl->seq);
    }

    if (tx_ctrl->remote_seq != rx_ctrl.seq) {
      tx_ctrl->remote_seq = rx_ctrl.seq;
      tx_ctrl.Update();
    }

    ProcessRx();
  }

  void Tick(float delta_time_s) {
    tx_ctrl.Tick(delta_time_s);
    tx_data_buffer.Tick(delta_time_s);
  }
};
}  // namespace control_stream

class RoboBus {
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
    /* upper_stream_->OnRx([this](uint32_t id, std::vector<uint8_t> const &data)
    { ProcessMessage(id, data);
    }); */

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
};
}  // namespace robobus::robobus
