#pragma once

#include <string>
#include <string_view>

#include <chrono>

template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
class SharedContext;

template <typename Clock>
class DebugInfo {
 public:
  explicit DebugInfo(SharedContext<Clock> ctx, std::string const& tag)
      : ctx_(ctx), tag_(tag) {}

  auto Message(std::string_view message) -> void {
    auto adapter = ctx_.Root().lock()->GetDebugAdapter();
    if (adapter.has_value()) {
      adapter.value()->Message(tag_, message);
    }
  }

 private:
  SharedContext<Clock> ctx_;
  std::string tag_;
};