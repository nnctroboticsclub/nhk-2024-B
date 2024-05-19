
#include <mbed.h>

#include <sstream>
#include <vector>
#include <iomanip>
#include <string>
#include <queue>
#include <variant>
#include <functional>
#include <unordered_map>

#include <ctype.h>

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
  *(volatile int*)0 = 0;
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

  ~Result() {}

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

class Random {
  AnalogIn source_{PC_0};

  float value_;

  void RandomThread() {
    while (1) {
      value_ *= 2;
      value_ += source_.read();
      value_ = fmod(value_, 1.0f);
      ThisThread::sleep_for(1ms);
    }
  }

  Random() {
    Thread* thread = new Thread(osPriorityNormal, 1024, nullptr, "Random");
    thread->start(callback(this, &Random::RandomThread));
  }
  static inline Random* instance;

 public:
  static Random* GetInstance() {
    if (!instance) {
      instance = new Random();
    }
    return instance;
  }

  static uint8_t GetByte() {
    return 0;
    return Random::GetInstance()->value_ * 255;
  }
};

namespace robotics::logger {
struct LogLine {
  size_t length = 0;
  char data[512] = {};

  enum class Level {
    kError,
    kInfo,
    kVerbose,
    kDebug,
    kTrace
  } level = Level::kInfo;

  LogLine(const char* data = nullptr, Level level = Level::kInfo)
      : level(level) {
    if (data) {
      memset(this->data, 0, sizeof(this->data));
      memcpy(this->data, data, strlen(data));
      length = strlen(this->data);
    }
  }

  char* operator=(const char* str) {
    memset(data, 0, sizeof(data));
    memcpy(data, str, strlen(str));
    length = strlen(data);
    return data;
  }
};

using Level = LogLine::Level;

using LogQueue = NoMutexLIFO<LogLine, 64>;

LogQueue* log_queue = nullptr;

Thread logger_thread;

void Log(Level level, const char* fmt, ...) {
  if (!log_queue) return;

  va_list args;
  va_start(args, fmt);

  LogLine line{"", level};
  line.length = vsnprintf(line.data, sizeof(line.data), fmt, args);

  log_queue->Push(line);

  va_end(args);
}

void LogHex(Level level, const uint8_t* data, size_t length) {
  if (!log_queue) return;

  LogLine line{"", level};
  line.length = 0;

  for (size_t i = 0; i < length; i++) {
    line.length += snprintf(line.data + line.length,
                            sizeof(line.data) - line.length, "%02X", data[i]);
  }

  log_queue->Push(line);
}

struct CharLogGroup {
  char group_tag[16] = {};
  char data[512] = {};

  Level level = Level::kInfo;

  enum class CachingMode {
    kNone,
    kNewLine,
  } caching_mode = CachingMode::kNewLine;

  void SetLevel(Level level) { this->level = level; }

  void SetCachingMode(CachingMode mode) { caching_mode = mode; }

  void AppendChar(char c) {
    if (caching_mode == CachingMode::kNone) {
      Log(level, "[Char#%s] %s", group_tag, data);
    }

    if (caching_mode == CachingMode::kNewLine) {
      if (c == '\n' || strlen(data) >= sizeof(data) - 1) {
        Log(level, "[Char#%s] %s", group_tag, data);
        memset(data, 0, sizeof(data));
        return;
      } else if (c == '\r') {
        return;
      } else if (isprint(c)) {
        data[strlen(data)] = c;
      } else {
        data[strlen(data)] = '\\';
        data[strlen(data)] = 'x';
        data[strlen(data)] = "0123456789ABCDEF"[c >> 4];
        data[strlen(data)] = "0123456789ABCDEF"[c & 0xF];
      }
    }
  }
};

CharLogGroup groups[16] = {};

static CharLogGroup* GetCharLogGroup(const char* group_tag) {
  for (auto&& group : groups) {
    if (strcmp(group.group_tag, group_tag) == 0) {
      return &group;
    }
  }

  for (auto&& group : groups) {
    if (strlen(group.group_tag) == 0) {
      strcpy(group.group_tag, group_tag);
      return &group;
    }
  }

  return nullptr;
}

void LogCh(const char* group_tag, char c) {
  auto group = GetCharLogGroup(group_tag);
  if (!group) {
    return;
  }

  group->AppendChar(c);
}

void Init() {
  printf("# Init Logger\n");
  if (log_queue) {
    printf("- Logger already initialized\n");
    return;
  }
  printf("- sizeof(LogQueue): %#08x\n", sizeof(LogQueue));
  log_queue = new LogQueue();
  while (!log_queue->Full()) {
    log_queue->Push({""});
  }

  while (!log_queue->Empty()) {
    log_queue->Pop();
  }
  printf("- Log Queue: %p\n", log_queue);

  logger_thread.start([]() {
    char level_header[12];
    // '\x1b', '[', '1', ';',
    // '3', x, 'm', ch
    // '\x1b', '[', 'm', '\0'

    while (1) {
      if (!log_queue) {
        ThisThread::sleep_for(1ms);
        continue;
      }

      auto queue_length = log_queue->Size();

      for (int i = 0; !log_queue->Empty(); i++) {
        auto line = log_queue->Pop();

        switch (line.level) {
          case Level::kError:
            snprintf(level_header, sizeof(level_header), "\x1b[1;31mE\x1b[m");
            break;
          case Level::kInfo:
            snprintf(level_header, sizeof(level_header), "\x1b[1;32mI\x1b[m");
            break;
          case Level::kVerbose:
            snprintf(level_header, sizeof(level_header), "\x1b[1;34mV\x1b[m");
            break;
          case Level::kDebug:
            snprintf(level_header, sizeof(level_header), "\x1b[1;36mD\x1b[m");
            break;
          case Level::kTrace:
            snprintf(level_header, sizeof(level_header), "\x1b[1;35mT\x1b[m");
            break;

          default:
            snprintf(level_header, sizeof(level_header), "\x1b[1;37m?\x1b[m");
            break;
        }

        printf("[\x1b[33mLOG %2d -%2d]\x1b[m %s %s\n", i, log_queue->Size(),
               level_header, line.data);
      }
      ThisThread::sleep_for(1ms);
    }
  });
  printf("- Logger Thread: %p\n", &logger_thread);

  printf("  - free cells: %d\n", log_queue->Size());

  log_queue->Push(LogLine("Logger Initialized"));
  Log(Level::kInfo, "Logger Initialized (from Log)");
}
}  // namespace robotics::logger

namespace robotics::network {
template <typename T, typename D = void, typename L = uint32_t>
class Stream {
  using OnReceiveCallback = std::function<void(D, T*, L)>;

 protected:
  std::vector<OnReceiveCallback> on_receive_callbacks_;

  void DispatchOnReceive(D ctx, std::vector<T> const& data) {
    for (auto&& cb : on_receive_callbacks_) {
      if (!cb) {
        continue;
      }
      cb(ctx, data);
    }
  }
  void DispatchOnReceive(D ctx, T* data, L length) {
    for (auto&& cb : on_receive_callbacks_) {
      if (!cb) {
        continue;
      }
      cb(ctx, data, length);
    }
  }

 public:
  virtual void Send(D ctx, T* data, L length) = 0;
  void Send(D ctx, std::vector<T> const& data) {
    Send(ctx, data.data(), data.size());
  }
  void OnReceive(OnReceiveCallback cb) {
    this->on_receive_callbacks_.emplace_back(cb);
  }
};

template <typename T>
class Stream<T, void> {
  using OnReceiveCallback = std::function<void(char* data, uint32_t length)>;

 protected:
  std::vector<OnReceiveCallback> on_receive_callbacks_;

  void DispatchOnReceive(std::vector<T> const& data) {
    for (auto&& cb : on_receive_callbacks_) {
      cb(data);
    }
  }
  void DispatchOnReceive(T* data, uint32_t length) {
    for (auto&& cb : on_receive_callbacks_) {
      cb(data);
    }
  }

 public:
  virtual void Send(T* data, uint32_t length) = 0;
  virtual void Send(std::vector<T> const& data) = 0;
  void OnReceive(OnReceiveCallback cb) {
    this->on_receive_callbacks_.emplace_back(cb);
  }
};

class Checksum {
  uint16_t current = 0;

 public:
  void Reset() { current = 0; }

  void operator<<(uint16_t x) {
    current = ((current & 0x007F) << 9) | ((current & 0xff80) >> 7);
    current ^= 0x35ca;
    current ^= x;
  }

  void operator<<(uint8_t x) { *this << ((uint16_t)((x << 8) | x)); }

  uint16_t Get() const { return current; }
};

class FEP_RawDriver : public Stream<uint8_t, uint8_t> {
  mbed::UnbufferedSerial serial_;

  NoMutexLIFO<char, 64> rx_queue_;

  NoMutexLIFO<uint16_t, 32> trans_log_queue;

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

  uint8_t on_binary_data_buffer[128];

  class DriverError : public std::string {
   public:
    DriverError() : std::string() {}

    DriverError(std::string const& message) : std::string(message) {}
  };

  void Send(std::string const& data) {
    serial_.write(data.data(), data.size());

    for (size_t i = 0; i < data.size(); i++) {
      logger::LogCh("FEP-TX", data[i]);
    }
  }

  void ISR_ParseBinary() {
    if (rx_queue_.Size() < 9) {
      return;
    }

    uint8_t address;
    uint32_t length;

    address = (rx_queue_[3] - '0') * 100 + (rx_queue_[4] - '0') * 10 +
              (rx_queue_[5] - '0');

    length = (rx_queue_[6] - '0') * 100 + (rx_queue_[7] - '0') * 10 +
             (rx_queue_[8] - '0');

    // Checks if there is enough data in the queue
    if (rx_queue_.Size() < 9 + length + 2) {
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

    DispatchOnReceive(address, on_binary_data_buffer, length);

    state_ = State::kIdle;
  }

  void ISR_OnUARTData() {
    static char buffer[1024];

    auto length = serial_.read(buffer, 1024);

    logger::LogCh("FEP-RX", buffer[0]);

    for (int i = 0; i < length; i++) {
      trans_log_queue.Push((0x00 << 8) | buffer[i]);

      if (!rx_queue_.Push(buffer[i])) {
        flags_ |= (uint32_t)Flags::kRxOverflow;
      }
    }

    if (rx_queue_[0] == 'R' && rx_queue_[1] == 'B' && rx_queue_[2] == 'N') {
      state_ = State::kRxData;
      ISR_ParseBinary();
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
      if (value == 0) return -10;
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
  }

  void WatchDebugQueue() {
    if (trans_log_queue.Empty()) {
      ThisThread::sleep_for(1ms);
      return;
    }

    auto e = trans_log_queue.Pop();

    if ((e >> 8) == 0x80) {
      // printf("TX: %02x\n", e & 0xFF);
    } else if ((e >> 8) == 0x00) {
      // printf("RX: %02x\n", e & 0xFF);
    } else if ((e >> 8) == 0x40) {
      // printf("D1: %02X\n", e & 0xFF);
    } else if ((e >> 8) == 0x41) {
      // printf("D2: %03X\n", e & 0xFF);
    } else if ((e >> 8) == 0x42) {
      // printf("D3: %02X\n", e & 0xFF);
    } else if ((e >> 8) == 0x43) {
      // printf("D4: %02X\n", e & 0xFF);
    } else {
      // printf("???: %02X\n", e & 0xFF);
    }
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

  [[nodiscard]]
  Result<int, DriverError> InitAllRegister() {
    auto wait_for_state = WaitForState(State::kIdle);
    if (!wait_for_state.IsOk()) {
      return wait_for_state.UnwrapError();
    }
    state_ = State::kProcessing;

    Send("@INI\r\n");

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
  void Send(uint8_t address, uint8_t* data, uint32_t length) override {
    auto wait_for_state = WaitForState(State::kIdle);
    if (!wait_for_state.IsOk()) {
      state_ = State::kIdle;
      return;
    }
    state_ = State::kProcessing;

    std::stringstream ss;
    ss << "@TBN";
    ss << std::setw(3) << std::setfill('0') << (int)address;
    ss << std::setw(3) << std::setfill('0') << (int)length;
    for (size_t i = 0; i < length; i++) {
      ss << (char)data[i];
    }
    ss << "\r\n";

    Send(ss.str());

    auto result1 = ReadResult();
    if (!result1.IsOk()) {
      state_ = State::kIdle;
      return;
    }

    if (result1.Unwrap() != +1) {
      logger::Log(logger::Level::kError,
                  "[FEP] \x1b[31mSend\x1b[m Invalid Response: %d",
                  result1.Unwrap());
      state_ = State::kIdle;
      return;
    }

    auto result2 = ReadResult();
    if (!result2.IsOk()) {
      logger::Log(logger::Level::kError,
                  "[FEP] \x1b[31mSend\x1b[m Failed to read result: %s",
                  result2.UnwrapError().c_str());
      state_ = State::kIdle;
      return;
    }

    if (result2.Unwrap() == -10) {
      logger::Log(logger::Level::kError,
                  "[FEP] \x1b[31mSend\x1b[m Invalid Command");
      state_ = State::kIdle;
      auto result = this->Reset();
      if (!result.IsOk()) {
        logger::Log(logger::Level::kError,
                    "[FEP] \x1b[31mSend\x1b[m Failed to reset: %s",
                    result.UnwrapError().c_str());
        return;
      } else {
        logger::Log(logger::Level::kInfo,
                    "[FEP] \x1b[31mSend\x1b[m Resetted FEP");
      }
      return;
    }
    if (result2.Unwrap() == -1) {
      logger::Log(logger::Level::kError,
                  "[FEP] \x1b[31mSend\x1b[m Failed due to no responce or CS");
      state_ = State::kIdle;
      return;
    }
    if (result2.Unwrap() == -2) {
      logger::Log(logger::Level::kError,
                  "[FEP] \x1b[31mSend\x1b[m Remote RX Overflow");
      state_ = State::kIdle;
      return;
    }

    state_ = State::kIdle;
  }
};

struct REPTxPacket {
  uint8_t no_key_check;
  uint8_t addr;
  uint8_t buffer[32];
  uint32_t length;
};

class ReliableFEPProtocol : public Stream<uint8_t, uint8_t> {
  FEP_RawDriver& driver_;

  uint8_t random_key_;

  Checksum rx_cs_calculator;
  Checksum tx_cs_calculator;

  uint8_t tx_buffer_[32] = {};
  struct {
    uint8_t initialized = 0;
    uint8_t key = 0;
  } cached_keys[256] = {};  // Using array for ISR context

  NoMutexLIFO<REPTxPacket, 4> tx_queue;

  void ExchangeKey_RequestImmediately(uint8_t remote) {
    logger::Log(logger::Level::kVerbose, "[REP] Key Request: to %d (Imm)",
                remote);

    REPTxPacket* packet = new REPTxPacket{true, remote, {Random::GetByte()}, 1};
    _Send(*packet);
  }
  void ExchangeKey_Request(uint8_t remote) {
    logger::Log(logger::Level::kVerbose, "[REP] Key Request: to %d", remote);

    this->tx_queue.Push({true, remote, {Random::GetByte()}, 1});
  }

  void ExchangeKey_Responce(uint8_t remote, uint8_t random) {
    this->tx_queue.Push(
        {true, remote, {random, (uint8_t)(random ^ random_key_)}, 2});

    logger::Log(logger::Level::kVerbose,
                "[REP] Key Responce: for %d, random=%#02x", remote, random);
  }

  void ExchangeKey_Update(uint8_t remote, uint8_t key) {
    cached_keys[remote] = {true, key};

    logger::Log(logger::Level::kVerbose, "[REP] Key Updated: for %d, key=%#02x",
                remote, key);
  }

  void _Send(REPTxPacket& packet) {
    if (!packet.no_key_check && !cached_keys[packet.addr].initialized) {
      logger::Log(logger::Level::kDebug,
                  "[REP] Packet marked as key_check, but the cached key is not "
                  "initialized for %d, Queueing exchanging...",
                  packet.addr);
      ExchangeKey_Request(packet.addr);

      return;
    }

    logger::Log(logger::Level::kDebug,
                "[REP] \x1b[31mSend\x1b[m key = %d(%d), data = %p (%d B) -> %d",
                cached_keys[packet.addr].key,
                cached_keys[packet.addr].initialized, packet.buffer,
                packet.length, packet.addr);

    auto key = cached_keys[packet.addr].key;

    tx_cs_calculator.Reset();
    tx_cs_calculator << (uint8_t)key;
    tx_cs_calculator << (uint8_t)packet.length;
    for (size_t i = 0; i < packet.length; i++) {
      tx_cs_calculator << (uint8_t)packet.buffer[i];
    }

    auto ptr = tx_buffer_;
    *(ptr++) = 0x55;
    *(ptr++) = 0xAA;
    *(ptr++) = 0xCC;
    *(ptr++) = key;
    *(ptr++) = packet.length;
    for (size_t i = 0; i < packet.length; i++) {
      *(ptr++) = packet.buffer[i];
    }

    *(ptr++) = (uint8_t)(tx_cs_calculator.Get() >> 8);
    *(ptr++) = (uint8_t)(tx_cs_calculator.Get() & 0xFF);

    driver_.Send(packet.addr, tx_buffer_, ptr - tx_buffer_);
  }

 public:
  ReliableFEPProtocol(FEP_RawDriver& driver) : driver_(driver) {
    this->random_key_ = Random::GetByte();

    logger::Log(logger::Level::kInfo, "[REP] Using Random Key: \e[1;32m%d\e[m",
                random_key_);

    driver_.OnReceive([this](uint8_t addr, uint8_t* data, size_t len) {
      //* Validate magic
      if (data[0] != 0x55 || data[1] != 0xAA || data[2] != 0xCC) {
        logger::Log(logger::Level::kError, "[REP] Invalid Magic: %d", addr);
        return;  // invalid magic
      }

      data += 3;

      //* Load key/length
      if (len <= 5) {
        logger::Log(logger::Level::kError, "[REP] Invalid Length (1): %d",
                    addr);
        return;  // malformed packet
      }

      auto key = *(data++);
      auto length = *(data++);
      if (len != 7 + length) {
        logger::Log(logger::Level::kError, "[REP] Invalid Length (2): %d",
                    addr);
        return;  // malformed packet
      }

      //* Load payload/len
      uint8_t* payload = data;
      uint32_t payload_len = length;
      data += payload_len;

      //* Validate Checksum
      rx_cs_calculator.Reset();
      rx_cs_calculator << (uint8_t)key;
      rx_cs_calculator << (uint8_t)length;
      for (size_t i = 0; i < payload_len; i++) {
        rx_cs_calculator << (uint8_t)payload[i];
      }

      uint16_t checksum = (*(data++) << 8) | *(data++);
      if (checksum != (uint16_t)rx_cs_calculator.Get()) {
        logger::Log(logger::Level::kError, "[REP] Invalid Checksum: %d", addr);
        return;  // invalid checksum
      }

      if (0)
        logger::Log(logger::Level::kDebug,
                    "[REP] RX: %d, key = %d, length = %d", addr, key, length);

      if (key == 0) {
        if (payload_len == 1)  // key request
        {
          ExchangeKey_Responce(addr, payload[0]);
          return;
        } else if (payload_len == 2)  // key responce
        {
          ExchangeKey_Update(addr, payload[0] ^ payload[1]);
          return;
        }
      }

      if (key != random_key_) {
        logger::Log(logger::Level::kError, "[REP] Invalid Key: %d", addr);
        ExchangeKey_Responce(addr, Random::GetByte());
        return;  // invalid key
      }

      DispatchOnReceive(addr, payload, payload_len);
    });

    (new Thread(osPriorityNormal, 8192, nullptr, "REP"))->start([this]() {
      while (1) {
        if (tx_queue.Empty()) {
          ThisThread::sleep_for(100ms);
          continue;
        }

        auto packet = tx_queue.Pop();
        logger::Log(logger::Level::kDebug,
                    "[REP] Consuming packet %d/%d (%d, byte[%d] to %d)", 1,
                    tx_queue.Size(), packet.no_key_check, packet.length,
                    packet.addr);

        if (!packet.no_key_check && !cached_keys[packet.addr].initialized) {
          logger::Log(
              logger::Level::kDebug,
              "[REP] Key not initialized: %d, skipping consume the queue...",
              packet.addr);
          ExchangeKey_Request(packet.addr);
          tx_queue.Push(packet);

          continue;
        }

        logger::Log(
            logger::Level::kDebug,
            "[REP] \x1b[33mSend\x1b[m key = %d(%d), data = %p (%d B) -> %d",
            cached_keys[packet.addr].key, cached_keys[packet.addr].initialized,
            packet.buffer, packet.length, packet.addr);
        _Send(packet);
      }
    });
  }

  void Send(uint8_t address, uint8_t* data, uint32_t length) override {
    static REPTxPacket packet;
    packet = REPTxPacket{false, address, {}, length};
    for (size_t i = 0; i < length; i++) {
      packet.buffer[i] = data[i];
    }
    tx_queue.Push(packet);
  }
};

class SSP_Service : public Stream<uint8_t, uint8_t> {
  Stream<uint8_t, uint8_t>& stream_;

  uint16_t service_id_;

 private:
  friend class SerialServiceProtocol;

 public:
  SSP_Service(Stream<uint8_t, uint8_t>& stream, uint16_t service_id)
      : stream_(stream), service_id_(service_id) {}

  void Send(uint8_t address, uint8_t* data, uint32_t length) override {
    static uint8_t buffer[128] = {};

    buffer[0] = (service_id_ >> 8) & 0xFF;
    buffer[1] = service_id_ & 0xFF;
    buffer[2] = 0;
    buffer[3] = 0;

    for (size_t i = 0; i < length; i++) {
      buffer[4 + i] = data[i];
    }

    stream_.Send(address, buffer, length + 4);
  }

  uint16_t GetServiceId() const { return service_id_; }
};

class SerialServiceProtocol {
  Stream<uint8_t, uint8_t>& stream_;
  std::unordered_map<uint16_t, SSP_Service*> services_;

 public:
  SerialServiceProtocol(Stream<uint8_t, uint8_t>& stream) : stream_(stream) {
    stream_.OnReceive(
        [this](uint8_t from_addr, uint8_t* data, uint32_t length) {
          if (length < 4) {
            robotics::logger::Log(logger::Level::kError,
                                  "[SSP] E: Invalid Length: %d", length);
            return;
          }

          uint16_t service_id = (data[0] << 8) | data[1];

          if (services_.find(service_id) == services_.end()) {
            robotics::logger::Log(logger::Level::kError,
                                  "[SSP] E: Invalid Service: %d", service_id);
            return;
          }
          auto service = services_[service_id];

          service->DispatchOnReceive(from_addr, data + 4, length - 4);
        });
  }

  template <typename T>
  void RegisterService() {
    auto service = new T(stream_);
    services_[service->GetServiceId()] = service;
  }
};
}  // namespace robotics::network

class EchoService : public robotics::network::SSP_Service {
 public:
  EchoService(robotics::network::Stream<uint8_t, uint8_t>& stream)
      : SSP_Service(stream, 0x0001) {
    OnReceive([this](uint8_t addr, uint8_t* data, size_t len) {
      robotics::logger::Log(robotics::logger::Level::kInfo, "[Echo] RX: %d",
                            addr);
      robotics::logger::LogHex(robotics::logger::Level::kInfo, data, len);
      robotics::logger::Log(robotics::logger::Level::kInfo, "[Echo] TX: %d",
                            addr);
      robotics::logger::LogHex(robotics::logger::Level::kInfo, data, len);
      Send(addr, data, len);
    });
  }
};
class Echo2Service : public robotics::network::SSP_Service {
 public:
  Echo2Service(robotics::network::Stream<uint8_t, uint8_t>& stream)
      : SSP_Service(stream, 0x0002) {
    OnReceive([this](uint8_t addr, uint8_t* data, size_t len) {
      robotics::logger::Log(robotics::logger::Level::kInfo, "[Echo2] RX: %d",
                            addr);
      robotics::logger::LogHex(robotics::logger::Level::kInfo, data, len);
      robotics::logger::Log(robotics::logger::Level::kInfo, "[Echo2] TX: %d",
                            addr);
      robotics::logger::LogHex(robotics::logger::Level::kInfo, data, len);

      if (len > 0) addr = data[0];
      Send(addr, data + 1, len - 1);
    });
  }
};

using robotics::logger::Level;
class App {
  robotics::network::FEP_RawDriver fep_;
  robotics::network::ReliableFEPProtocol rep_;
  robotics::network::SerialServiceProtocol ssp_;

  void SetupFEP(int mode) {
    auto fep_address = mode == 1 ? 2 : 1;
    auto group_address = 0xF0;
    robotics::logger::Log(Level::kInfo, "# Setup FEP");
    robotics::logger::Log(Level::kInfo, "- Mode         : %d", mode);
    robotics::logger::Log(Level::kInfo, "- Addr         : %d", fep_address);
    robotics::logger::Log(Level::kInfo, "- Group Address: %d", group_address);

    {
      auto res = fep_.InitAllRegister();
      if (!res.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              res.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto result = fep_.Reset();
      if (!result.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              result.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto res = fep_.SetRegister(0, fep_address);
      if (!res.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              res.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto res = fep_.SetRegister(1, group_address);
      if (!res.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              res.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto res = fep_.SetRegister(18, 0x8D);
      if (!res.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              res.UnwrapError().c_str());
        while (1);
      }
    }

    {
      auto res = fep_.Reset();
      if (!res.IsOk()) {
        robotics::logger::Log(Level::kError, "FAILED!!!: %s",
                              res.UnwrapError().c_str());
        while (1);
      }
    }
  }

  void Init() {
    rep_.OnReceive([this](uint8_t addr, uint8_t* data, size_t len) {
      robotics::logger::Log(Level::kInfo, "[App] I: REP-RX: %d", addr);
      robotics::logger::LogHex(Level::kInfo, data, len);
      robotics::logger::Log(Level::kInfo, "[App]");
    });
  }

  void Watcher() {
    while (1) {
      fep_.WatchDebugQueue();

      ThisThread::sleep_for(10ms);
    }
  }

  int GetMode() {
    mbed::DigitalIn mode(PA_9);

    return mode.read() ? 1 : 0;
  }

 public:
  App() : fep_{PB_6, PA_10, 9600}, rep_{fep_}, ssp_(rep_) {}

  void Main() {
    auto mode = GetMode();

    robotics::logger::Init();

    SetupFEP(mode);
    Init();

    ssp_.RegisterService<EchoService>();
    ssp_.RegisterService<Echo2Service>();

    auto dest_address = mode == 1 ? 1 : 2;
    robotics::logger::Log(Level::kInfo, "[App] Dest Address: %d\n",
                          dest_address);

    (new Thread(osPriorityNormal, 1024, nullptr, "Watcher"))
        ->start(callback(this, &App::Watcher));

    std::vector<uint8_t> data = {'A', 'B', 'C'};

    while (1) {
      // rep.Send(dest_address, data.data(), data.size());

      ThisThread::sleep_for(2s);
    }
  }
};

int main() {
  printf("# main()\n");
  printf("- Build info\n");
  printf("  - Date: %s\n", __DATE__);
  printf("  - Time: %s\n", __TIME__);

  App* app = new App();

  app->Main();

  return 0;
}
