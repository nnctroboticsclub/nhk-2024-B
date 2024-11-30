#pragma once

#include <string>
#include "render.hpp"

template <render::Renderee R>
class DebugNode {
 public:
  DebugNode(uint32_t id, std::string const& name, R const& renderable)
      : id_(id), name_(name), renderable_(renderable) {}

  void Render(render::BaseRenderer& renderer) {
    renderable_.RenderTo(renderer);
  }

 private:
  uint32_t id_;
  std::string name_;
  R renderable_;
};