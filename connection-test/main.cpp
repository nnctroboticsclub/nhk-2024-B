#include <cstdio>

#include <chrono>
#include <memory>

#include <logger/logger.hpp>
#include <robotics/random/random.hpp>
#include <robotics/thread/thread.hpp>

// #include "im920_test->hpp"
// #include "can_debug.hpp"
#include "robobus/robo-bus.hpp"

using robobus::robobus::RoboBus;
using robobus::types::DataCtrlMarker;
using robobus::types::DeviceID;
using robobus::types::MessageID;

class ControlStreamOnCAN {
  static inline robotics::logger::Logger logger_{"cstream@can",
                                                 "ControlStreamOnCAN"};
  std::shared_ptr<robotics::network::CANBase> can_;
  std::unique_ptr<robobus::robobus::control_stream::ControlStreamCore> st_ =
      std::make_unique<robobus::robobus::control_stream::ControlStreamCore>();

  MessageID tx_ctrl_msg_id;
  MessageID rx_ctrl_msg_id;
  MessageID tx_data_msg_id;
  MessageID rx_data_msg_id;

 public:
  robobus::internal::SignalRx<int> tx_empty;

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
        rx_data_msg_id(config.rx_data_msg_id),
        tx_empty(st_->tx_empty) {
    logger_.Info("ControlStreamOnCAN will initialized with following config:");
    logger_.Info("  tx_ctrl_msg_id: %d", tx_ctrl_msg_id.GetMsgID());
    logger_.Info("  rx_ctrl_msg_id: %d", rx_ctrl_msg_id.GetMsgID());
    logger_.Info("  tx_data_msg_id: %d", tx_data_msg_id.GetMsgID());
    logger_.Info("  rx_data_msg_id: %d", rx_data_msg_id.GetMsgID());
    st_->tx_ctrl.updated.Connect([this, config](auto data) {
      can_->Send(tx_ctrl_msg_id.GetMsgID(), data.ToCANData());
    });

    st_->tx_data_buffer.updated.Connect([&](std::vector<uint8_t> const &data) {
      can_->Send(tx_data_msg_id.GetMsgID(), data);
    });

    st_->rx_available.Connect([&](std::vector<uint8_t> const &data) {
      static char buf[80];
      for (size_t i = 0; i < data.size(); i++) {
        snprintf(buf + i * 3, 80 - i * 3, "%02X ", data[i]);
      }
      logger_.Info("    DATA %s", buf);
    });

    can_->OnRx([this, config](uint32_t id, std::vector<uint8_t> const &data) {
      auto msg_id = MessageID(id);
      if (msg_id == rx_ctrl_msg_id) {
        st_->FeedRxCtrl(data);
      } else if (msg_id == rx_data_msg_id) {
        st_->FeedRxData(data);
      }
    });
  }

  inline void FeedTxData(
      robobus::robobus::control_stream::CANData const &data) {
    st_->FeedTxData(data);
  }

  inline void Tick(float delta_time_s) { st_->Tick(delta_time_s); }
};

class RoboBusTest {
  static inline robotics::logger::Logger logger_{"test->robo-bus.nw",
                                                 "RoboBusTest"};

  bool is_motherboard_ = false;

  robotics::network::SimpleCAN simple_can{PB_8, PB_9, (int)1E6};
  std::shared_ptr<robotics::network::CANBase> can_;
  std::unique_ptr<robobus::robobus::RoboBus> robobus_;

  std::shared_ptr<ControlStreamOnCAN> cstream_;

  void Test() {
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

    robobus::robobus::control_stream::CANData data;
    data.resize(8);
    data[0] = role == Role::kServer ? 0x10 : 0x20;

    using namespace std::chrono_literals;
    robotics::system::Timer timer;
    timer.Start();

    int i = 0;

    const float kFeedInterval_s = 5;
    float feed_timer = kFeedInterval_s;
    bool can_feed = true;
    auto Feed = [&] {
      if (can_feed && feed_timer <= 0) {
        data[6] = i >> 8;
        data[7] = i & 0xFF;
        i++;
        st.FeedTxData(data);
        can_feed = false;
        feed_timer = kFeedInterval_s;
      }
    };

    st.tx_empty.Connect([&](int) {
      can_feed = true;
      Feed();
    });

    while (true) {
      auto delta_time = timer.ElapsedTime();  // microseconds
      auto delta_time_s = (float)delta_time.count() * 1E-6f;
      timer.Reset();

      st.Tick(delta_time_s);

      feed_timer -= delta_time_s;
      Feed();

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

    logger_.Info("Device ID   : %d", device_id.GetDeviceID());
    logger_.Info("Motherboard?: %d", is_motherboard_);

    simple_can.SetCANExtended(true);  // RoboBus uses extended ID

    can_ = std::shared_ptr<robotics::network::CANBase>(&simple_can);

    robobus_ = std::make_unique<RoboBus>(device_id, is_motherboard_, can_);
    robobus_->DebugCAN();

    Test();
  }
};

int main() {
  using namespace std::chrono_literals;

  printf("# main()\n");

  robotics::system::Random::GetByte();
  robotics::system::SleepFor(20ms);
  robotics::logger::core::Init();

  printf("Starting App...\n");

  auto app = std::make_unique<RoboBusTest>();
  app->Main();
}
