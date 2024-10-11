#pragma once

#include <cstdint>

#include <array>
#include <unordered_map>

#include <robotics/network/simple_can.hpp>
#include <robotics/thread/thread.hpp>

struct CanMessageData {
  static uint8_t last_line;

  bool is_invalidated = true;
  uint8_t line = last_line;
  std::array<std::uint8_t, 8> data = {};
  uint8_t length = 0;

  int rx_count = 0;

  static void AddLine() { last_line++; }
};

uint8_t CanMessageData::last_line = 0;

class CanDebug {
  const int kHeaderLines = 3;

  robotics::network::SimpleCAN can_{PA_11, PA_12, (int)1E6};

  std::unordered_map<uint32_t, CanMessageData> messages_;
  uint32_t messages_count_ = 0;
  int tick_ = 0;
  int last_failed_tick_ = 0;

  bool printf_lock_ = false;

  void InitScreen() const {
    printf("\x1b[2J\x1b[1;1H");
    printf("\x1b[?25l");
  }

  void UpdateScreen() {
    while (printf_lock_) {
      robotics::system::SleepFor(1ms);
    }
    printf_lock_ = true;

    for (auto&& [id, data] : messages_) {
      if (!data.is_invalidated) {
        continue;
      }

      printf("\x1b[%d;1H", kHeaderLines + 1 + data.line);
      printf("%8d] %08lX (%5u):", tick_, id, data.rx_count);
      for (size_t i = 0; i < data.length; i++) {
        printf(" %02X", data.data[i]);
      }
      printf("\x1b[0K\n");

      data.is_invalidated = false;
    }

    printf_lock_ = false;
  }

  void ShowHeader() {
    while (printf_lock_) {
      robotics::system::SleepFor(1ms);
    }

    printf_lock_ = true;
    printf("\x1b[1;1H");
    printf("\x1b[2K");
    printf("Tick: %5d\x1b[0K\n", tick_);
    printf("Messages: %5ld\x1b[0K\n", messages_count_);
    printf("Last Failed: %5d\x1b[0K\n", last_failed_tick_);
    printf_lock_ = false;
  }

  void Init() {
    InitScreen();
    can_.OnRx([this](uint32_t id, std::vector<uint8_t> data) {
      if (messages_.find(id) == messages_.end()) {
        messages_[id] = CanMessageData();
        messages_count_++;
        CanMessageData::AddLine();
      }

      auto&& msg = messages_[id];
      msg.is_invalidated = true;
      msg.rx_count++;
      msg.length = static_cast<uint8_t>(data.size());
      std::copy(data.begin(), data.end(), msg.data.begin());
    });
  }

  void TestSend() {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    auto ret = can_.Send(0x400, data);

    if (ret != 1) {
      last_failed_tick_ = tick_;
    }
  }

 public:
  [[noreturn]]
  void Main() {
    Init();
    can_.Init();

    while (true) {
      UpdateScreen();
      ShowHeader();
      tick_++;
      robotics::system::SleepFor(100ms);

      TestSend();
    }
  }
};
