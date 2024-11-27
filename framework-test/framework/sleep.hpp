#pragma once

#include <chrono>
#include <thread>

#include <robotics/thread/thread.hpp>

#include "loop.hpp"

/// @brief Sleep を行うための awaiter
template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
struct SleepAwaiter {
  Loop<Clock> &ctx;
  Clock::duration duration;

  explicit SleepAwaiter(Loop<Clock> &ctx, Clock::duration duration)
      : ctx(ctx), duration(duration) {}

  // Sleep が実行済かどうか
  // 今回は常に false を返す
  //  (Sleep した後は true
  //  になる．しかし，その時コルーチンは廃棄されているので)．
  bool await_ready() const { return false; }

  void await_suspend(std::coroutine_handle<> handle) {
    ctx.RequestResumeAt(ctx.time.Now() + duration, handle);
  }

  void await_resume() const { return; }
};

/// @brief Robobus Loop を用いて Sleep を行う
/// @tparam Clock Clock の型
/// @param ctx Loop のコンテキスト
/// @param duration Sleep する時間
/// @return Awaiter
template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
auto Sleep(Loop<Clock> &ctx, typename Clock::duration duration) {
  return SleepAwaiter(ctx, duration);
}