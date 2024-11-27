#pragma once

#include <coroutine>
#include <optional>
#include <type_traits>
#include <vector>
#include <functional>

//* Forward declaration
template <typename ReturnType>
struct Coroutine;

template <typename ReturnType>
struct Promise;

//* BasePromise
template <typename ReturnType>
class BasePromise {
 protected:
  void Return() const {
    for (auto &callback : on_return_callbacks) {
      callback();
    }
  }

 public:
  void AddOnReturnCallback(std::function<void()> callback) {
    on_return_callbacks.emplace_back(callback);
  }

  operator Promise<ReturnType> &() {
    return *static_cast<Promise<ReturnType> *>(this);
  }

 public:
  auto get_return_object() {
    return Coroutine<ReturnType>{
        std::coroutine_handle<Promise<ReturnType>>::from_promise(*this)};
  }

  auto initial_suspend() { return std::suspend_never{}; }

  auto final_suspend() noexcept { return std::suspend_never{}; }

  auto unhandled_exception() { std::terminate(); }

 private:
  std::vector<std::function<void()>> on_return_callbacks;
};

//* Promise
template <std::move_constructible ReturnType>
struct Promise<ReturnType> : public BasePromise<ReturnType> {
 public:
  auto get_return_value() { return value_; }

 public:
  auto return_value(ReturnType value) {
    this->value_ = std::move(value);
    this->Return();

    return std::suspend_never{};
  }

 private:
  std::optional<ReturnType> value_;
};

template <>
struct Promise<void> : public BasePromise<void> {
 public:
  auto get_return_value() { return finished; }

 public:
  auto return_void() {
    finished = true;
    this->Return();

    return std::suspend_never{};
  }

 private:
  bool finished = false;
};