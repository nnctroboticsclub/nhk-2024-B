#include <cstdio>

#include <chrono>
#include <memory>
#include <coroutine>
#include <list>

#include <logger/logger.hpp>
#include <robotics/random/random.hpp>
#include <robotics/thread/thread.hpp>

#include <robo-bus.hpp>
#include "../platform.hpp"

namespace apps::robobus_test {
using robobus::internal::MultiUpdatable;
using robobus::internal::Signal;
using robobus::internal::SignalRx;
using robobus::internal::SignalTx;
using robobus::types::DataCtrlMarker;
using robobus::types::DeviceID;
using robobus::types::MessageID;
using robotics::Node;
using robotics::logger::Logger;
using robotics::network::RD16;

using CANDataType = std::vector<uint8_t>;

/// @brief 制御ストリームの状態符号
class ControlStreamState {
  uint32_t data_ = 0;

 public:
  explicit ControlStreamState(uint32_t data = 0) : data_(data) {}

  void operator=(uint32_t data) { data_ = data; }

  /// @brief データを混ぜ合わせる (32bit 幅)
  /// @param code 混ぜ合わせるコード
  void MixCode(uint32_t code) { data_ = (data_ * code + code) ^ (data_ >> 16); }

  /// @brief データを取得
  uint32_t Get() const { return data_; }

  /// @brief データを設定
  void Set(uint32_t data) { data_ = data; }
};

/// @brief 出力側の制御ストリーム
class ControlStream_Outbound {
  static inline Logger logger{"out.cstream.nw", "ConSt.\x1b[34mOut\x1b[m"};
  bool is_ok_value_ = false;
  SignalTx<int> is_ok_signal_{std::make_shared<Signal<int>>()};

  void CheckIntegrity() {
    if (is_ok_value_) {
      return;
    }

    if (!state_validate_.GetOptional().has_value()) {
      // logger.Debug("TX Not Ready");
      return;
    }

    if (state_validate_->Get() != state_feedback_.Get()) {
      // logger.Debug("TX Mismatch (%08X != %08X)", state_validate_->Get(),
      //              state_feedback_.Get());
      return;
    }

    // logger.Info("TX Ok");

    data_.Reset();
    is_ok_signal_.Fire(0);
    is_ok_value_ = true;
  }

  ControlStreamState state_feedback_{};

 public:
  MultiUpdatable<CANDataType> data_;
  MultiUpdatable<ControlStreamState> state_validate_{ControlStreamState{0}};
  SignalRx<int> is_ok_{is_ok_signal_};

 public:
  void SetData(CANDataType data) {
    // logger.Info("Feed data");
    is_ok_value_ = false;
    data_.GetOptional().emplace(data);
    data_.Update();

    state_validate_->MixCode(RD16::FromData(data).Get());
    // logger.Info("  +- h_ov: %08X", state_validate_->Get());
    state_validate_.Update();
  }

  void SetStateFeedback(ControlStreamState state) {
    // logger.Info("Tx State Feedback (%08X)", state.Get());
    is_ok_value_ = false;
    state_feedback_ = state;

    CheckIntegrity();
  }

  void Tick(float delta_time_s) {
    data_.Tick(delta_time_s);
    state_validate_.Tick(delta_time_s);
  }
};

/// @brief 入力側の制御ストリーム
class ControlStream_Inbound {
  static inline Logger logger{"out.cstream.nw", "ConSt.\x1b[35mIn\x1b[m "};

  //* Caches
  ControlStreamState previous_state_v{0};
  ControlStreamState calculated_state_{0};
  bool is_ok_value_{false};
  bool has_new_data_{false};
  SignalTx<int> is_ok_signal_{std::make_shared<Signal<int>>()};
  SignalTx<CANDataType> data_signal_{std::make_shared<Signal<CANDataType>>()};

  void CheckIntegrity() {
    if (is_ok_value_) {
      return;
    }

    if (!has_new_data_) {
      // logger.Debug("RX Not Ready");
      return;
    }

    if (state_feedback->Get() == calculated_state_.Get()) {
      // logger.Debug("RX Same (%08X)", state_feedback->Get());
      return;
    }

    if (state_validate.Get() != calculated_state_.Get()) {
      // logger.Info("RX Ng (%08X != %08X)", state_validate.Get(),
      // calculated_state_.Get());
      return;
    }

    // logger.Info("RX Ok");

    /* logger.Debug("h_if: %08x => %08x", state_feedback->Get(),
                 calculated_state_.Get()); */

    state_feedback.GetOptional() = calculated_state_;
    state_feedback.Update();

    data_signal_.Fire(data);

    is_ok_signal_.Fire(0);
    is_ok_value_ = true;
    has_new_data_ = false;
  }

  CANDataType data;
  ControlStreamState state_validate{0};

 public:
  MultiUpdatable<ControlStreamState> state_feedback{ControlStreamState{0}};
  SignalRx<CANDataType> data_{data_signal_};
  SignalRx<int> is_ok_{is_ok_signal_};

 public:
  void FeedData(CANDataType const &data) {
    // logger.Trace("Feed data");
    has_new_data_ = true;
    is_ok_value_ = false;

    this->data = data;
    calculated_state_ = previous_state_v;
    calculated_state_.MixCode(RD16::FromData(data).Get());

    CheckIntegrity();
  }

  void SetStateValidate(ControlStreamState state) {
    // logger.Trace("Rx State Validate (%08X)", state.Get());
    if (state.Get() == state_validate.Get()) {
      // logger.Debug("State is same (Ignore)");
      return;
    }

    is_ok_value_ = false;
    previous_state_v = state_validate;
    state_validate = state;
    CheckIntegrity();
  }

  void Tick(float delta_time_s) { state_feedback.Tick(delta_time_s); }
};

/// @brief 制御ストリームに置ける制御データ
using ControlData = CANDataType;

/// @brief 制御ストリーム
class ControlStream {
  static inline Logger logger{"cstream.nw", "ConSt    "};

  ControlStream_Inbound inbound_;
  ControlStream_Outbound outbound_;

  SignalTx<ControlData> tx_ctrl_signal_{
      std::make_shared<Signal<CANDataType>>()};

  void ProcessTxControlData() {
    ControlData ret;
    auto h_ov = outbound_.state_validate_->Get();
    auto h_if = inbound_.state_feedback->Get();
    // logger.Debug("Tx Control Data: %08X %08X", h_ov, h_if);

    ret.resize(8);
    ret[0] = (h_ov >> 24) & 0xff;
    ret[1] = (h_ov >> 16) & 0xff;
    ret[2] = (h_ov >> 8) & 0xff;
    ret[3] = (h_ov >> 0) & 0xff;
    ret[4] = (h_if >> 24) & 0xff;
    ret[5] = (h_if >> 16) & 0xff;
    ret[6] = (h_if >> 8) & 0xff;
    ret[7] = (h_if >> 0) & 0xff;

    tx_ctrl_signal_.Fire(ret);
  }

 public:
  SignalRx<ControlData> tx_ctrl{tx_ctrl_signal_};
  SignalRx<CANDataType> tx_data{outbound_.data_.updated};
  SignalRx<CANDataType> rx_data{inbound_.data_};

  SignalRx<int> rx_ok_{inbound_.is_ok_};
  SignalRx<int> tx_ok_{outbound_.is_ok_};

  ControlStream() {
    outbound_.state_validate_.updated.Connect(
        [this](ControlStreamState const &) { ProcessTxControlData(); });
    inbound_.state_feedback.updated.Connect(
        [this](ControlStreamState const &) { ProcessTxControlData(); });
  }

  void LoadRxControlData(ControlData data) {
    auto h_iv = (uint32_t(data[0]) << 24) | (uint32_t(data[1]) << 16) |
                (uint32_t(data[2]) << 8) | (uint32_t(data[3]));

    auto h_of = (uint32_t(data[4]) << 24) | (uint32_t(data[5]) << 16) |
                (uint32_t(data[6]) << 8) | (uint32_t)(data[7]);

    inbound_.SetStateValidate(ControlStreamState{h_iv});
    outbound_.SetStateFeedback(ControlStreamState{h_of});
  }

  void PutTxData(CANDataType const &data) { outbound_.SetData(data); }

  void FeedRxData(CANDataType const &data) { inbound_.FeedData(data); }

  void Tick(float delta_time_s) {
    inbound_.Tick(delta_time_s);
    outbound_.Tick(delta_time_s);
  }
};

/// @brief CAN 上での制御ストリーム
class ControlStreamOnCAN {
  static inline Logger logger{"can.cstream.nw", "ConSt@CAN"};

  std::shared_ptr<robotics::network::CANBase> can_;
  std::unique_ptr<ControlStream> st_ = std::make_unique<ControlStream>();

  MessageID tx_ctrl_msg_id;
  MessageID rx_ctrl_msg_id;
  MessageID tx_data_msg_id;
  MessageID rx_data_msg_id;

 public:
  SignalRx<ControlData> tx_ctrl{st_->tx_ctrl};
  SignalRx<CANDataType> tx_data{st_->tx_data};
  SignalRx<CANDataType> rx_data{st_->rx_data};

  SignalRx<int> rx_ok_{st_->rx_ok_};
  SignalRx<int> tx_ok_{st_->tx_ok_};

  struct Config {
    std::shared_ptr<robotics::network::CANBase> upper_can_;
    MessageID tx_ctrl_msg_id;
    MessageID rx_ctrl_msg_id;
    MessageID tx_data_msg_id;
    MessageID rx_data_msg_id;
  };

  explicit ControlStreamOnCAN(Config const &config)
      : can_(config.upper_can_),
        tx_ctrl_msg_id(config.tx_ctrl_msg_id),
        rx_ctrl_msg_id(config.rx_ctrl_msg_id),
        tx_data_msg_id(config.tx_data_msg_id),
        rx_data_msg_id(config.rx_data_msg_id) {
    logger.Info("ControlStreamOnCAN will initialized with following config:");
    logger.Info("  tx_ctrl_msg_id: %d", tx_ctrl_msg_id.GetMsgID());
    logger.Info("  rx_ctrl_msg_id: %d", rx_ctrl_msg_id.GetMsgID());
    logger.Info("  tx_data_msg_id: %d", tx_data_msg_id.GetMsgID());
    logger.Info("  rx_data_msg_id: %d", rx_data_msg_id.GetMsgID());
    st_->tx_ctrl.Connect([this](auto const &data) {
      // logger.Info("==> \x1b[34mtx\x1b[m \x1b[32mctrl\x1b[m");
      // logger.HexInfo(data.data(), data.size());
      can_->Send(tx_ctrl_msg_id.GetMsgID(), data);
    });

    st_->tx_data.Connect([this](auto const &data) {
      // logger.Info("==> \x1b[34mtx\x1b[m \x1b[33mdata\x1b[m");
      // logger.HexInfo(data.data(), data.size());
      can_->Send(tx_data_msg_id.GetMsgID(), data);
    });

    can_->OnRx([this](uint32_t id, std::vector<uint8_t> const &data) {
      auto msg_id = MessageID(id);
      if (msg_id == rx_ctrl_msg_id) {
        // logger.Info("<== \x1b[35mrx\x1b[m \x1b[32mctrl\x1b[m");
        // logger.HexInfo(data.data(), data.size());
        st_->LoadRxControlData(data);
      } else if (msg_id == rx_data_msg_id) {
        // logger.Info("<== \x1b[35mrx\x1b[m \x1b[33mdata\x1b[m");
        // logger.HexInfo(data.data(), data.size());
        st_->FeedRxData(data);
      } else {
        // logger.Info("<== \x1b[35m???\x1b[m [ID=%08lx]", id);
        // logger.HexInfo(data.data(), data.size());
      }
    });
  }

  inline void FeedTxData(CANDataType const &data) { st_->PutTxData(data); }

  inline void Tick(float delta_time_s) { st_->Tick(delta_time_s); }
};

/// @brief テスト
class RoboBusTest {
  static inline Logger logger{"test->robo-bus.nw", "RoboBusTest"};

  bool is_motherboard_ = false;

  robotics::network::SimpleCAN simple_can{PB_8, PB_9, (int)1E6};
  std::shared_ptr<robotics::network::CANBase> can_;
  // std::unique_ptr<robobus::robobus::RoboBus> robobus_;

  std::shared_ptr<ControlStreamOnCAN> cstream_;

  void Test() const {
    auto cpipe_dev_id = DeviceID(1);

    enum class Role {
      kServer,
      kClient,
    };
    auto role = is_motherboard_ ? Role::kServer : Role::kClient;

    ControlStreamOnCAN st(ControlStreamOnCAN::Config{
        .upper_can_ = can_,
        .tx_ctrl_msg_id = MessageID::CreateControlTransfer(
            cpipe_dev_id, role == Role::kServer ? DataCtrlMarker::kServerCtrl
                                                : DataCtrlMarker::kClientCtrl),
        .rx_ctrl_msg_id = MessageID::CreateControlTransfer(
            cpipe_dev_id, role == Role::kServer ? DataCtrlMarker::kClientCtrl
                                                : DataCtrlMarker::kServerCtrl),
        .tx_data_msg_id = MessageID::CreateControlTransfer(
            cpipe_dev_id, role == Role::kServer ? DataCtrlMarker::kServerData
                                                : DataCtrlMarker::kClientData),
        .rx_data_msg_id = MessageID::CreateControlTransfer(
            cpipe_dev_id, role == Role::kServer ? DataCtrlMarker::kClientData
                                                : DataCtrlMarker::kServerData),
    });

    CANDataType data;
    data.resize(8);
    data[0] = role == Role::kServer ? 0x10 : 0x20;

    using namespace std::chrono_literals;
    robotics::system::Timer timer;
    timer.Start();

    int i = 0;

    bool can_feed = true;
    auto Feed = [&] {
      if (can_feed) {
        data[6] = i >> 8;
        data[7] = i & 0xFF;
        i++;
        st.FeedTxData(data);
        can_feed = false;
      }
    };

    st.tx_ok_.Connect([&can_feed, Feed](int) {
      can_feed = true;
      Feed();
    });

    struct DebugInfo {
      uint16_t value = 0;
      uint16_t rx_count = 0;
    };

    DebugInfo info;

    st.rx_data.Connect([&info](CANDataType const &data) {
      if (data.size() != 8) {
        logger.Error("Invalid data size: %d", data.size());
        return;
      }

      info.value = (data[6] << 8) | data[7];
      info.rx_count++;
    });

    static const float kDebugInfoInterval_s = 0.2f;
    float debug_timer = 0;
    uint16_t prev_value = 0;

    while (true) {
      auto delta_time = timer.ElapsedTime();  // microseconds
      auto delta_time_s = (float)delta_time.count() * 1E-6f;
      timer.Reset();

      st.Tick(delta_time_s);

      Feed();

      debug_timer -= delta_time_s;
      if (debug_timer <= 0) {
        auto delta_value = info.value - prev_value;
        auto delta_time = kDebugInfoInterval_s - debug_timer;
        auto speed = delta_value / delta_time;
        prev_value = info.value;

        logger.Info("v: %3d, Δv: %3d, Δt: %4.2f, Transfer Speed: %4.2f",
                    info.value, delta_value, delta_time, speed);

        debug_timer = kDebugInfoInterval_s;
      }

      robotics::system::SleepFor(1ms);
    }
  }

 public:
  void Main() {
    using enum platform::Mode;

    auto mode = platform::GetMode();

    DeviceID device_id{0};
    switch (mode) {
      case kDevice1:
        device_id = DeviceID(0);
        is_motherboard_ = true;
        break;
      case kDevice2:
        device_id = DeviceID(1);
        is_motherboard_ = false;
        break;
    }

    logger.Info("Device ID   : %d", device_id.GetDeviceID());
    logger.Info("Motherboard?: %d", is_motherboard_);

    simple_can.SetCANExtended(true);  // RoboBus uses extended ID
    simple_can.Init();

    can_ = std::shared_ptr<robotics::network::CANBase>(&simple_can);

    // robobus_ = std::make_unique<RoboBus>(device_id, is_motherboard_, can_);
    // robobus_->DebugCAN();

    Test();
  }
};
}  // namespace apps::robobus_test

namespace apps {
using RoboBusTest = apps::robobus_test::RoboBusTest;
}
