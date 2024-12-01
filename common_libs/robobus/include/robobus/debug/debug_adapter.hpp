#pragma once

#include <string_view>

namespace robobus::debug {
class DebugAdapter {
 public:
  virtual ~DebugAdapter() = default;

  virtual void Message(std::string_view path, std::string_view text) = 0;
};
}  // namespace robobus::debug