#pragma once

#include <string>
#include <string_view>

#include <chrono>

namespace robobus::context {
template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
class SharedContext;
}

namespace robobus::debug {

template <typename Clock>
class DebugInfo {
 public:
  explicit DebugInfo(context::SharedContext<Clock> ctx, std::string const& tag)
      : ctx_(ctx), tag_(tag) {}

  auto Message(std::string_view message) -> void {
    auto adapter = ctx_.Root().lock()->GetDebugAdapter();
    if (adapter.has_value()) {
      adapter.value()->Message(tag_, message);
    }
  }

 private:
  context::SharedContext<Clock> ctx_;
  std::string tag_;
};

}  // namespace robobus::debug
