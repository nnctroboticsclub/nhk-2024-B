#include <iostream>
#include <coroutine>
#include <list>
#include <chrono>

#include <robotics/thread/thread.hpp>
#include <logger/logger.hpp>

using robotics::logger::Logger;

template <typename ClockT>
  requires std::chrono::is_clock_v<ClockT>
class TimeContext {
  static inline Logger logger{"time.context.nw", "Ctx  Time"};

  using time_point = ClockT::time_point;
  using duration = ClockT::duration;

  ClockT clock_;

  time_point start_;
  time_point now_;
  time_point last_;
  duration delta_;

 public:
  TimeContext() { Reset(); }

  auto Reset() -> void {
    start_ = ClockT::now();

    now_ = start_;
    last_ = start_;
    delta_ = ClockT::duration::zero();
  }

  auto Tick() -> void {
    last_ = now_;
    now_ = ClockT::now();
    delta_ = now_ - last_;
  }

  auto ElapsedTime() const -> duration { return now_ - start_; }

  auto DeltaTime() const -> duration { return delta_; }

  auto Started() const -> time_point { return start_; }

  auto Now() const -> time_point { return now_; }
};

/// @brief コルーチンベースプログラムで用いるコンテキスト
class Context {
  static inline Logger logger{"context.nw", "Context "};
  std::list<std::coroutine_handle<>> coroutines_;

 public:
  TimeContext<std::chrono::steady_clock> time;

  void AddTask(std::coroutine_handle<> coroutine) {
    coroutines_.push_back(coroutine);
  }

  void Run() {
    while (!coroutines_.empty()) {
      for (auto &&coroutine : coroutines_) {
        if (coroutine.done()) {
          logger.Info("Remove the %p", coroutine.address());
          coroutines_.remove(coroutine);
        }

        logger.Info("Resume the %p", coroutine.address());
        coroutine.resume();

        time.Tick();
      }
    }
  }
};

struct Coroutine {
  struct promise_type;
  using coro_handle = std::coroutine_handle<promise_type>;

  struct promise_type {
    auto get_return_object() {
      return Coroutine{coro_handle::from_promise(*this)};
    }

    auto initial_suspend() { return std::suspend_never{}; }

    auto final_suspend() noexcept { return std::suspend_never{}; }

    auto return_void() {}

    auto unhandled_exception() { std::terminate(); }
  };

  coro_handle handle;

  auto await_ready() { return false; }

  auto await_suspend(std::coroutine_handle<> awaiting) {
    handle.resume();
    return false;
  }

  auto await_resume() {}
};

struct Sleep_Awaitable {
  std::chrono::microseconds duration;

  Sleep_Awaitable(std::chrono::microseconds duration) : duration(duration) {}

  bool await_ready() const { return false; }

  void await_suspend(std::coroutine_handle<> handle) {
    robotics::system::SleepFor(
        std::chrono::duration_cast<std::chrono::milliseconds>(duration));
    handle.resume();
  }

  void await_resume() {}
};

class ContextTest {
  Sleep_Awaitable Sleep(Context &ctx,
                        std::chrono::steady_clock::duration duration) {
    return Sleep_Awaitable(
        std::chrono::duration_cast<std::chrono::microseconds>(duration));
  }

  Coroutine Task1(Context &ctx) {
    while (true) {
      printf("Task1\n");
      co_await Sleep(ctx, std::chrono::seconds(1));
    };
  }

  Coroutine Task2(Context &ctx) {
    co_await Sleep(ctx, std::chrono::milliseconds(250));
    while (true) {
      printf("Task1\n");
      co_await Sleep(ctx, std::chrono::seconds(1));
    };
  }

  Coroutine Task3(Context &ctx) {
    co_await Sleep(ctx, std::chrono::milliseconds(500));
    while (true) {
      printf("Task1\n");
      co_await Sleep(ctx, std::chrono::seconds(1));
    };
  }

 public:
  void Test() {
    Context ctx;

    ctx.AddTask(Task1(ctx).handle);
    ctx.AddTask(Task2(ctx).handle);
    ctx.AddTask(Task3(ctx).handle);
    ctx.Run();
  }
};

int main() {
  using namespace std::chrono_literals;

  robotics::system::SleepFor(20ms);
  robotics::logger::core::Init();

  ContextTest test;
  test.Test();

  return 0;
}