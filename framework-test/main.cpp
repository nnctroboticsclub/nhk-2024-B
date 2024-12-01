#include <chrono>
#include <iostream>

#include <fmt/format.h>

#include "framework/context.hpp"
#include "framework/base_coroutine.hpp"

#include "test_clock.hpp"
#include "measurement.hpp"

using Clock = TestClock;

using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;

class SimpleDebugAdapter : public DebugAdapter {
 public:
  void Message(std::string_view path, std::string_view text) override {
    std::cout << path << ": " << text << std::endl;
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

  robotics::logger::SuppressLogger("loop.robobus");

  auto ctx = SharedRootContext<Clock>();
  // ctx.GetLoop().LaunchDebugThread();
  ctx.SetDebugAdapter(std::make_shared<SimpleDebugAdapter>());

  LaunchLoopTask(ctx.Child("Loop"));

  ctx.Run();

  return 0;
}
