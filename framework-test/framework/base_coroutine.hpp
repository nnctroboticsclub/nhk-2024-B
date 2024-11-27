#pragma once

#include <coroutine>
#include <optional>
#include <type_traits>

//* Forward declaration
template <typename ReturnType>
struct Coroutine;

template <typename ReturnType>
struct Promise;

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

//* Promise

template <std::move_constructible ReturnType>
struct Promise<ReturnType> {
 private:
  void Return() const {
    for (auto &callback : on_return_callbacks) {
      callback();
    }
  }

 public:
  void AddOnReturnCallback(std::function<void()> callback) {
    on_return_callbacks.emplace_back(callback);
  }

  auto get_return_value() { return value_; }

 public:
  auto get_return_object() {
    return Coroutine<ReturnType>{
        std::coroutine_handle<Promise<ReturnType>>::from_promise(*this)};
  }

  auto initial_suspend() { return std::suspend_never{}; }

  auto final_suspend() noexcept { return std::suspend_never{}; }

  auto unhandled_exception() { std::terminate(); }

  auto return_value(ReturnType value) {
    this->value_ = std::move(value);
    this->Return();

    return std::suspend_never{};
  }

 private:
  std::vector<std::function<void()>> on_return_callbacks;

  std::optional<ReturnType> value_;
};

template <>
struct Promise<void> {
 private:
  void Return() const {
    for (auto &callback : on_return_callbacks) {
      callback();
    }
  }

 public:
  void AddOnReturnCallback(std::function<void()> callback) {
    on_return_callbacks.emplace_back(callback);
  }

  auto get_return_value() { return finished; }

 public:
  auto get_return_object() {
    return Coroutine<void>{
        std::coroutine_handle<Promise<void>>::from_promise(*this)};
  }

  auto initial_suspend() { return std::suspend_never{}; }

  auto final_suspend() noexcept { return std::suspend_never{}; }

  [[noreturn]] auto unhandled_exception() { std::terminate(); }

  auto return_void() {
    finished = true;
    this->Return();

    return std::suspend_never{};
  }

 private:
  std::vector<std::function<void()>> on_return_callbacks;
  bool finished = false;
};

//* CoroutineAwaiter
template <std::move_constructible ReturnType>
struct CoroutineAwaiter<ReturnType> {
  explicit CoroutineAwaiter(Promise<ReturnType> &promise) : promise(promise) {}

  bool await_ready() const { return (bool)promise.get_return_value(); }
  void await_suspend(std::coroutine_handle<> handle) {
    promise.AddOnReturnCallback([handle]() { handle.resume(); });
  }
  auto await_resume() const { return std::move(promise.get_return_value()); }

 private:
  Promise<ReturnType> &promise;
};

template <>
struct CoroutineAwaiter<void> {
  explicit CoroutineAwaiter(Promise<void> &promise) : promise(promise) {}

  bool await_ready() const { return promise.get_return_value(); }
  void await_suspend(std::coroutine_handle<> handle) {
    promise.AddOnReturnCallback([handle]() { handle.resume(); });
  }
  auto await_resume() const { return; }

 private:
  Promise<void> &promise;
};