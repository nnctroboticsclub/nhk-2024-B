#include <chrono>

#include <robotics/thread/thread.hpp>
#include <thread>

#include "framework/context.hpp"
#include "framework/base_coroutine.hpp"
#include "test_clock.hpp"
#include "measurement.hpp"

using robotics::logger::Logger;
using Clock = TestClock;
using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;

Coroutine<void> Task(SharedContext<Clock> ctx, int number,
                     std::chrono::milliseconds delay) {
  auto logger = ctx.Logger();

  while (true) {
    logger.Info("%*c#", 3 * number, ' ');
    co_await ctx.Sleep(delay);
  }

  co_return;
}

void LaunchLoopTask(SharedContext<Clock> ctx) {
  ctx.AddTask(Task(ctx, 1, 2000ms / (1)).handle);
  ctx.AddTask(Task(ctx, 2, 2000ms / (2)).handle);
  ctx.AddTask(Task(ctx, 3, 2000ms / (3)).handle);
  ctx.AddTask(Task(ctx, 4, 2000ms / (4)).handle);
  ctx.AddTask(Task(ctx, 5, 2000ms / (5)).handle);
  ctx.AddTask(Task(ctx, 6, 2000ms / (6)).handle);
  ctx.AddTask(Task(ctx, 7, 2000ms / (7)).handle);
  ctx.AddTask(Task(ctx, 8, 2000ms / (8)).handle);
  ctx.AddTask(Task(ctx, 9, 2000ms / (9)).handle);
}

int main() {
  using namespace std::chrono_literals;

  robotics::system::SleepFor(20ms);
  robotics::logger::core::Init();

  robotics::logger::SuppressLogger("loop.robobus");

  auto ctx = SharedRootContext<Clock>();
  ctx.GetLoop().LaunchDebugThread();

  LaunchLoopTask(ctx.Child("Loop"));

  ctx.Run();

  return 0;
}