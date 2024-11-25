#include <chrono>

#include <robotics/thread/thread.hpp>
#include <thread>

#include "framework/context.hpp"
#include "framework/base_coroutine.hpp"
#include "test_clock.hpp"

using robotics::logger::Logger;

using Clock = TestClock;

BaseCoroutine Task(SharedContext<Clock> ctx, int number,
                   std::chrono::milliseconds offset) {
  co_await ctx.Sleep(std::chrono::milliseconds(100));
  co_await ctx.Sleep(offset);
  ctx.Logger().Info("Task%d started (%s)", number, ctx.ContextId().c_str());
  while (true) {
    ctx.Logger().Info("Task%d", number);
    co_await ctx.Sleep(std::chrono::seconds(1));
  }

  co_return;
}

int main() {
  using namespace std::chrono_literals;

  robotics::system::SleepFor(20ms);
  robotics::logger::core::Init();

  auto ctx = SharedRootContext<Clock>();
  // ctx.GetLoop().LaunchDebugThread();

  robotics::logger::SuppressLogger("loop.robobus");

  ctx.AddTask(Task(ctx.Child("Task1"), 1, std::chrono::milliseconds(0)).handle);
  ctx.AddTask(Task(ctx.Child("Task2"), 2, std::chrono::milliseconds(1)).handle);
  ctx.AddTask(
      Task(ctx.Child("Task3"), 3, std::chrono::milliseconds(250)).handle);
  ctx.AddTask(
      Task(ctx.Child("Task4"), 4, std::chrono::milliseconds(500)).handle);

  ctx.Run();

  return 0;
}