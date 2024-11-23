#pragma once

#include <coroutine>

struct BaseCoroutine {
  struct promise_type;
  using coro_handle = std::coroutine_handle<promise_type>;

  struct promise_type {
    auto get_return_object() {
      return BaseCoroutine{coro_handle::from_promise(*this)};
    }

    auto initial_suspend() { return std::suspend_never{}; }

    auto final_suspend() noexcept { return std::suspend_never{}; }

    auto return_void() {}

    auto unhandled_exception() { std::terminate(); }
  };

  coro_handle handle;
};