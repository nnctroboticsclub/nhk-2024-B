#include <chrono>

#include <robotics/thread/thread.hpp>
#include <thread>

#include "framework/context.hpp"
#include "framework/base_coroutine.hpp"
#include "test_clock.hpp"

using robotics::logger::Logger;
using Clock = TestClock;

Coroutine<int> Test_Proc(SharedContext<Clock> ctx) {
  using namespace std::chrono_literals;

  ctx.Sleep(100ms);

  co_return 53;
}
Coroutine<void> Test(SharedContext<Clock> ctx) {
  using namespace std::chrono_literals;
  auto logger = ctx.Logger();

  auto result = co_await Test_Proc(ctx.Child("Test_Proc"));

  logger.Info("Test: %d", result);

  co_return;
}

Coroutine<void> Task(SharedContext<Clock> ctx, int number,
                     std::chrono::milliseconds offset) {
  using namespace std::chrono_literals;
  auto logger = ctx.Logger();

  logger.Info("Task%d started (id=%s)", number, ctx.ContextId().c_str());

  co_await ctx.Sleep(offset + 100ms);
  while (true) {
    logger.Info("Task%d", number);
    co_await ctx.Sleep(1s);
  }

  co_return;
}

void LaunchLoopTask(SharedContext<Clock> ctx) {
  using namespace std::chrono_literals;

  ctx.AddTask(Task(ctx.Child("Task1"), 1, 0ms).handle);
  ctx.AddTask(Task(ctx.Child("Task2"), 2, 1ms).handle);
  ctx.AddTask(Task(ctx.Child("Task3"), 3, 250ms).handle);
  ctx.AddTask(Task(ctx.Child("Task4"), 4, 500ms).handle);
}

int main() {
  using namespace std::chrono_literals;

  robotics::system::SleepFor(20ms);
  robotics::logger::core::Init();

  auto ctx = SharedRootContext<Clock>();
  ctx.GetLoop().LaunchDebugThread();

  robotics::logger::SuppressLogger("loop.robobus");

  LaunchLoopTask(ctx.Child("Loop"));

  ctx.AddTask(Test(ctx.Child("Test")).handle);

  ctx.Run();

  return 0;
}