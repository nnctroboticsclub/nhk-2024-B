#pragma once

#include <utility>
#include <vector>
#include <string>
#include <cstdint>

namespace render {

using ComponentByteCode = std::vector<std::uint8_t>;

class BaseRenderer {
 public:
  virtual ~BaseRenderer() = default;

  virtual void RenderHTML(std::string const& html) = 0;
  virtual void RenderComponent(ComponentByteCode const& component) = 0;
};

template <typename T>
concept Renderee = requires(T t) {
  { t.RenderTo(std::declval<BaseRenderer&>()) } -> std::same_as<void>;
};

}  // namespace render