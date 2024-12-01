#include <chrono>
#include <iostream>

#include <fmt/format.h>

#include <logger/log_sink.hpp>
#include <robotics/utils/no_mutex_lifo.hpp>

#include <robobus/context.hpp>
#include <robobus/base_coroutine.hpp>

#include <mbed.h>

class MbedClock {
 public:
  using duration = std::chrono::duration<float, std::milli>;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<MbedClock, duration>;

 private:
  static mbed::Timer timer_;

 public:
  static void Init() {
    timer_.reset();
    timer_.start();
  }

  static time_point now() {
    auto delta_ms = timer_.read_ms();
    auto duration = MbedClock::duration(delta_ms);

    return time_point(duration);
  }
};

mbed::Timer MbedClock::timer_;

template <>
struct std::chrono::is_clock<MbedClock> : std::true_type {};

template <>
const bool std::chrono::is_clock_v<MbedClock> = true;

using Clock = MbedClock;

using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;

class SimpleLogSink : public robotics::logger::LogSink {
 public:
  void Log(robotics::logger::core::Level level, const char* tag,
           const char* msg) override {
    std::cout << fmt::format("$1$log${:d}${:d}{:s}${:d}{:s}$\n",
                             static_cast<int>(level), strlen(tag), tag,
                             strlen(msg), msg);
  }
};

class SimpleDebugAdapter : public DebugAdapter {
 public:
  void Message(std::string_view path, std::string_view text) override {
    std::cout << fmt::format("$1$dbg${:d}{:s}${:d}{:s}$\n", path.size(), path,
                             text.size(), text);
  }
};

Coroutine<void> Task(SharedContext<Clock> ctx,
                     std::chrono::milliseconds delay) {
  auto logger = ctx.Logger();
  auto test_debug = ctx.GetDebugInfo("test_debug");

  logger.RenameTag("   Test  ");

  logger.Info("Started 'Task' (cid = %s)", ctx.ContextId().c_str());

  co_await ctx.Sleep(delay);

  while (true) {
    const auto now = ctx.GetLoop().time.Now().time_since_epoch();
    const auto now_s =
        std::chrono::duration_cast<std::chrono::seconds>(now).count();

    test_debug.Message(fmt::format("Time: {}", now_s));

    co_await ctx.Sleep(1s);
  }

  co_return;
}

void LaunchLoopTask(SharedContext<Clock> ctx) {
  ctx.AddTask(Task(ctx.Child("a"), 1000ms / 1).handle);
  ctx.AddTask(Task(ctx.Child("b"), 1000ms / 2).handle);
  ctx.AddTask(Task(ctx.Child("c"), 1000ms / 3).handle);
}

int main() {
  robotics::system::SleepFor(20ms);
  robotics::logger::core::Init();
  MbedClock::Init();

  robotics::logger::global_log_sink = new SimpleLogSink();
  robotics::logger::SuppressLogger("loop.robobus");

  auto ctx = SharedRootContext<Clock>();
  // ctx.GetLoop().LaunchDebugThread();
  ctx.SetDebugAdapter(std::make_shared<SimpleDebugAdapter>());

  LaunchLoopTask(ctx.Child("Loop"));

  ctx.Run();

  return 0;
}
