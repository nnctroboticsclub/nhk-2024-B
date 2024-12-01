#pragma once

#include <coroutine>
#include "promise.hpp"

namespace robobus::coroutine {
template <typename ReturnType>
struct CoroutineAwaiter;

//* Coroutine
template <typename ReturnType>
struct Coroutine {
  using coro_handle = std::coroutine_handle<Promise<ReturnType>>;
  using promise_type = Promise<ReturnType>;

  explicit Coroutine(coro_handle handle) : handle(handle) {}

  coro_handle handle;

  auto operator co_await() {
    return CoroutineAwaiter<ReturnType>{handle.promise()};
  }
};

}  // namespace robobus::coroutine