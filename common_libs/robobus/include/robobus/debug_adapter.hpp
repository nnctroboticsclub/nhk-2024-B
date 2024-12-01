#pragma once

#include <string_view>

class DebugAdapter {
 public:
  virtual ~DebugAdapter() = default;

  virtual void Message(std::string_view path, std::string_view text) = 0;
};