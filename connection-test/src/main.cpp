
#include <mbed.h>

#include <sstream>
#include <vector>
#include <iomanip>
#include <string>
#include <queue>
#include <variant>
#include <functional>

template <typename T, size_t N>
class NoMutexLIFO {
  /**
   * 0. Initial state
   *                         N
   * <----------------------------------------------->
   * +-----+-----+-----+-----+-----+-----+-----+-----+
   * |     |     |     |     |     |     |     |     |
   * +-----+-----+-----+-----+-----+-----+-----+-----+
   *    ^                                         ^
   *    | head_                                   | tail_
   *
   * 1. After Push(0)
   *
   * +-----+-----+-----+-----+-----+-----+-----+-----+
   * |  0  |     |     |     |     |     |     |     |
   * +-----+-----+-----+-----+-----+-----+-----+-----+
   *          ^                                   ^
   *          | head_                             | tail_
   *
   * Process:
   *   buffer_[head_] = data
   *   head_ = (head_ + 1) % N
   *
   * 2. After Push(0), Push(1), ..., Push(6)
   *
   * +-----+-----+-----+-----+-----+-----+-----+-----+
   * |  0  |  1  |  2  |  3  |  4  |  5  |  6  |     |
   * +-----+-----+-----+-----+-----+-----+-----+-----+
   *                                              ^
   *                                              | head_
   *                                              | tail_
   * head_ == tail_ -> buffer is full
   *
   *
   * 3. Pop()
   *
   * +-----+-----+-----+-----+-----+-----+-----+-----+
   * |     |  1  |  2  |  3  |  4  |  5  |  6  |     |
   * +-----+-----+-----+-----+-----+-----+-----+-----+
   *    ^                                         ^
   *    | tail_                                   | head_
   *
   * Process:
   *   tail_ = (tail_ + 1) % N
   *   auto data = buffer_[tail_]
   *
   */

  std::array<T, N> buffer_ = {};
  size_t head_ = 0;
  size_t tail_ = N - 1;

 public:
  NoMutexLIFO() {
    while (!Empty()) {
      Pop();
    }
  }

  bool Empty() const { return (N + tail_ - head_ + 1) % N == 0; }

  bool Full() const { return head_ == tail_; }

  bool Push(T const& data) {
    if (Full()) {
      return false;
    }

    buffer_[head_] = data;
    head_ = (head_ + 1) % N;

    return true;
  }

  T Pop() {
    if (Empty()) {
      return {};
    }

    tail_ = (tail_ + 1) % N;
    auto data = buffer_[tail_];

    return data;
  }

  T& operator[](size_t index) { return buffer_[(1 + tail_ + index) % N]; }

  size_t Size() const { return (N - (tail_ - head_) - 1) % N; }
};

[[noreturn]] void panic(const char* message) {
  printf("Panic: %s\n", message);
  while (1) {
  }
}

template <typename T, typename E>
class Result {
  enum class Tag {
    kOk,
    kError,
  };

  Tag tag_;

  T value_;
  E error_;

 public:
  Result(T value) : tag_(Tag::kOk), value_(value) {}

  Result(E error) : tag_(Tag::kError), error_(error) {}

  ~Result() {
    if (tag_ == Tag::kOk) {
      value_.~T();
    } else {
      error_.~E();
    }
  }

  T Unwrap() const {
    if (tag_ == Tag::kError) {
      panic("Result is Error");
    }
    return value_;
  }

  E UnwrapError() const {
    if (tag_ == Tag::kOk) {
      panic("Result is Ok");
    }
    return error_;
  }

  bool IsOk() const { return tag_ == Tag::kOk; }
};

class FEP_RawDriver {
  mbed::UnbufferedSerial serial_;

  NoMutexLIFO<char, 128> rx_queue_;

  NoMutexLIFO<uint16_t, 64> trans_log_queue;

  enum class Flags {
    kRxOverflow = 1 << 0,
  };

  uint32_t flags_ = 0;

  enum class State {
    kIdle,
    kProcessing,
    kRxData,
  };

  State state_ = State::kIdle;

  Timer timer_;

  // Must can be called from ISR context
  using OnBinaryDataCB =
      std::function<void(uint8_t data_from, char* data, size_t len)>;
  std::vector<OnBinaryDataCB> on_binary_data_callbacks_;

  char on_binary_data_buffer[128];

  class DriverError : public std::string {
   public:
    DriverError() : std::string() {}

    DriverError(std::string const& message) : std::string(message) {}
  };

  void Send(std::string const& data) {
    serial_.write(data.data(), data.size());

    for (size_t i = 0; i < data.size(); i++) {
      trans_log_queue.Push((0x80 << 8) | data[i]);
    }
  }

  void ISR_ParseBinary() {
    if (rx_queue_.Size() < 9) {
      trans_log_queue.Push((0x40 << 8) | 0xFE);
      return;
    }

    int address;
    int length;

    address = (rx_queue_[3] - '0') * 100 + (rx_queue_[4] - '0') * 10 +
              (rx_queue_[5] - '0');

    length = (rx_queue_[6] - '0') * 100 + (rx_queue_[7] - '0') * 10 +
             (rx_queue_[8] - '0');

    // Checks if there is enough data in the queue
    if (rx_queue_.Size() < 9 + length + 2) {
      trans_log_queue.Push((0x40 << 8) | 0xFF);
      return;
    }

    // Skips "RBN" + AAA + LLL, where AAA is the address and LLL is the length
    for (size_t i = 0; i < 9; i++) {
      rx_queue_.Pop();
    }

    for (int i = 0; i < length; i++) {
      on_binary_data_buffer[i] = rx_queue_.Pop();
    }

    // Skips the "\r\n" at the end of the binary data
    for (size_t i = 0; i < 2; i++) {
      rx_queue_.Pop();
    }

    for (auto& callback : on_binary_data_callbacks_) {
      callback(address, on_binary_data_buffer, length);
    }

    trans_log_queue.Push((0x40 << 8) | address);
    trans_log_queue.Push((0x41 << 8) | length);
    trans_log_queue.Push((0x42 << 8) | on_binary_data_buffer[0]);
    trans_log_queue.Push((0x43 << 8) | on_binary_data_buffer[1]);

    state_ = State::kIdle;
  }

  void ISR_OnUARTData() {
    auto length = serial_.readable();
    char buffer[length];

    serial_.read(buffer, length);

    for (int i = 0; i < length; i++) {
      trans_log_queue.Push((0x00 << 8) | buffer[i]);

      if (!rx_queue_.Push(buffer[i])) {
        flags_ |= (uint32_t)Flags::kRxOverflow;
      }
    }
    trans_log_queue.Push((0x01 << 8) | buffer[0]);

    if (rx_queue_[0] == 'R' && rx_queue_[1] == 'B' && rx_queue_[2] == 'N') {
      state_ = State::kRxData;
      ISR_ParseBinary();
    }
  }

  bool ISR_HasCompleteLine() {
    for (size_t i = 0; i < rx_queue_.Size(); i++) {
      if (rx_queue_[i] == '\n') {
        return true;
      }
    }
  }

  [[nodiscard]]
  Result<int, DriverError> WaitForState(
      State state, std::chrono::milliseconds timeout = 1000ms) {
    auto start = timer_.elapsed_time();

    while (1) {
      if (state_ == state) {
        return 0;
      }

      if (timer_.elapsed_time() > start + timeout) {
        return DriverError("State Timeout");
      }
    }
  }

  [[nodiscard]]
  Result<std::string, DriverError> ReadLine(
      std::chrono::milliseconds timeout = 1000ms) {
    std::string line;

    auto start = timer_.elapsed_time();

    while (1) {
      if (rx_queue_.Empty()) {
        if (timer_.elapsed_time() > start + timeout) {
          return DriverError("Timeout");
        }
        continue;
      }

      auto c = rx_queue_.Pop();
      if (c == '\r') {
        continue;
      }
      if (c == '\n') {
        break;
      }

      line.push_back(c);
    }

    return line;
  }

  [[nodiscard]]
  Result<int, DriverError> ReadResult(
      std::chrono::milliseconds timeout = 1000ms) {
    auto line_res = ReadLine(timeout);
    if (!line_res.IsOk()) {
      return line_res.UnwrapError();
    }

    auto line = line_res.Unwrap();
    int value;
    char c;
    std::stringstream ss;
    ss << line;
    ss >> c >> value;

    if (c == 'N') {
      return -value;
    } else if (c == 'P') {
      return value;
    } else {
      ss.str("");
      ss << "Invalid response: " << line;
      return DriverError(ss.str());
    }
  }

 public:
  FEP_RawDriver(PinName tx, PinName rx, int baud) : serial_(tx, rx, baud) {
    timer_.reset();
    timer_.start();
    serial_.attach([this]() { ISR_OnUARTData(); }, mbed::SerialBase::RxIrq);

    if (0)
      (new Thread())->start([this]() {
        while (1) {
          if (trans_log_queue.Empty()) {
            ThisThread::sleep_for(1ms);
            continue;
          }

          auto e = trans_log_queue.Pop();
          if ((e >> 8) == 0x80) {
            printf("TX: %c\n", e & 0xFF);
          } else if ((e >> 8) == 0x00) {
            printf("RX: %02x\n", e & 0xFF);
          } else if ((e >> 8) == 0x40) {
            printf("D1: %02X\n", e & 0xFF);
          } else if ((e >> 8) == 0x41) {
            printf("D2: %03X\n", e & 0xFF);
          } else if ((e >> 8) == 0x42) {
            printf("D3: %02X\n", e & 0xFF);
          } else if ((e >> 8) == 0x43) {
            printf("D4: %02X\n", e & 0xFF);
          } else {
            printf("???: %02X\n", e & 0xFF);
          }
        }
      });
  }

  void OnBinaryData(OnBinaryDataCB callback) {
    on_binary_data_callbacks_.push_back(callback);
  }

  [[nodiscard]]
  Result<uint8_t, DriverError> GetRegister(
      uint8_t address, std::chrono::milliseconds timeout = 1000ms) {
    auto wait_for_state = WaitForState(State::kIdle, timeout);
    if (!wait_for_state.IsOk()) {
      return wait_for_state.UnwrapError();
    }

    std::stringstream ss;
    ss << "@REG" << std::setw(2) << std::setfill('0') << (int)address << "\r\n";
    Send(ss.str());
    ss.str("");

    state_ = State::kProcessing;

    auto line_res = ReadLine(timeout);
    if (!line_res.IsOk()) {
      return line_res.UnwrapError();
    }

    auto line = line_res.Unwrap();
    int value;
    ss << std::hex << line;
    ss >> value;

    state_ = State::kIdle;
    return value;
  }

  [[nodiscard]]
  Result<int, DriverError> SetRegister(uint8_t address, uint8_t value) {
    auto wait_for_state = WaitForState(State::kIdle);
    if (!wait_for_state.IsOk()) {
      return wait_for_state.UnwrapError();
    }
    state_ = State::kProcessing;

    std::stringstream ss;
    ss << "@REG" << std::setw(2) << std::setfill('0') << (int)address << ":"
       << std::setw(3) << std::setfill('0') << (int)value << "\r\n";
    Send(ss.str());

    state_ = State::kIdle;
    return ReadResult();
  }

  [[nodiscard]]
  Result<int, DriverError> Reset() {
    auto wait_for_state = WaitForState(State::kIdle);
    if (!wait_for_state.IsOk()) {
      return wait_for_state.UnwrapError();
    }
    state_ = State::kProcessing;

    Send("@RST\r\n");

    state_ = State::kIdle;
    return ReadResult(1000ms);
  }

  enum class TxState_ { kNoError, kTimeout, kInvalidResponse, kRxOverflow };

  class TxState {
    TxState_ value_;

   public:
    TxState(TxState_ value) : value_(value) {}
    TxState() : value_(TxState_::kNoError) {}

    bool operator==(TxState const& other) const {
      return value_ == other.value_;
    }

    bool operator!=(TxState const& other) const {
      return value_ != other.value_;
    }
  };

  [[nodiscard]]
  Result<TxState, DriverError> SendBinary(uint8_t address,
                                          std::vector<uint8_t> const& data) {
    auto wait_for_state = WaitForState(State::kIdle);
    if (!wait_for_state.IsOk()) {
      state_ = State::kIdle;
      return wait_for_state.UnwrapError();
    }
    state_ = State::kProcessing;

    std::stringstream ss;
    ss << "@TBN";
    ss << std::setw(3) << std::setfill('0') << (int)address;
    ss << std::setw(3) << std::setfill('0') << data.size();
    for (auto d : data) {
      ss << (char)d;
    }
    ss << "\r\n";

    Send(ss.str());

    auto result1 = ReadResult();
    if (!result1.IsOk()) {
      state_ = State::kIdle;
      return result1.UnwrapError();
    }

    if (result1.Unwrap() != +1) {
      ss.str();
      ss << "Invalid response: " << result1.Unwrap();
      state_ = State::kIdle;
      return DriverError(ss.str());
    }

    auto result2 = ReadResult();
    if (!result2.IsOk()) {
      state_ = State::kIdle;
      return result2.UnwrapError();
    }

    if (result2.Unwrap() == 0) {
      state_ = State::kIdle;
      return TxState(TxState_::kNoError);
    }
    if (result2.Unwrap() == -1) {
      state_ = State::kIdle;
      return TxState(TxState_::kTimeout);
    }
    if (result2.Unwrap() == -2) {
      state_ = State::kIdle;
      return TxState(TxState_::kRxOverflow);
    }

    state_ = State::kIdle;
    return TxState(TxState_::kInvalidResponse);
  }
};

int main() {
  printf("main() started\n");
  printf("- Date: %s\n", __DATE__);
  printf("- Time: %s\n", __TIME__);
  FEP_RawDriver fep{PB_6, PA_10, 9600};
  struct {
    char buf[128];
    size_t len;
  } buf = {0};

  {
    auto ressr = fep.SetRegister(0, 1);
    if (!ressr.IsOk()) {
      panic(ressr.UnwrapError().c_str());
    }
  }

  {
    auto ressr = fep.SetRegister(18, 0x8D);
    if (!ressr.IsOk()) {
      panic(ressr.UnwrapError().c_str());
    }
  }

  {
    auto result = fep.Reset();
    if (!result.IsOk()) {
      panic(result.UnwrapError().c_str());
    }
  }

  fep.OnBinaryData([&buf](uint8_t addr, char* data, size_t len) {
    if (addr != 0) {
      return;
    }
    for (size_t i = 0; i < len; i++) {
      buf.buf[i] = data[i];
    }
    buf.len = len;
  });

  ThisThread::sleep_for(1s);
  std::vector<uint8_t> data = {'A', 'B'};

  while (1) {
    printf("RX Data: ");
    for (size_t i = 0; i < buf.len; i++) {
      printf("%c", buf.buf[i]);
    }
    printf("\n");
    auto result = fep.SendBinary(0xF0, data);
    if (!result.IsOk()) {
      panic(result.UnwrapError().c_str());
    }

    ThisThread::sleep_for(1s);
  }

  return 0;
}
