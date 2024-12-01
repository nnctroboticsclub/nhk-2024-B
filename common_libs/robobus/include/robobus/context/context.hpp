#pragma once

#include <optional>
#include <cstring>

#include <logger/logger.hpp>

#include "root_context.hpp"
#include "../debug/debug_info.hpp"
#include "../runtime/sleep.hpp"

namespace robobus::context {
template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
class SharedContext;

/// @brief コルーチンベースプログラムで用いるコンテキスト
template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
struct Context {
 private:
  std::string tag_;
  std::string path_;

  std::vector<SharedContext<Clock>> child_contexts_;

  std::weak_ptr<RootContext<Clock>> root_ctx;

  std::vector<debug::DebugInfo<Clock>> debug_infos_;
  std::optional<robotics::logger::Logger> logger;

 public:
  explicit Context(std::weak_ptr<RootContext<Clock>> root, std::string_view tag,
                   std::string_view parent_path)
      : root_ctx(root),
        tag_(tag),
        path_(std::string(parent_path) + "." + std::string(tag)) {}

  explicit Context(std::weak_ptr<RootContext<Clock>> root, std::string_view tag)
      : root_ctx(root), tag_(tag), path_(tag) {}

  auto Root() -> std::weak_ptr<RootContext<Clock>> { return root_ctx; }

  auto AddChild(SharedContext<Clock> sub_context) {
    child_contexts_.emplace_back(sub_context);
  }

  auto ContextId() -> std::string { return path_; }

  inline auto AddTask(std::coroutine_handle<> coroutine) -> void {
    Root().loop.AddTask(coroutine);
  }

  auto AddDebugInfo(debug::DebugInfo<Clock> debug_info) {
    debug_infos_.emplace_back(debug_info);
  }

  auto Logger() -> robotics::logger::Logger& {
    if (!logger.has_value()) {
      auto cid = ContextId();

      auto cid_cstr = new char[cid.size() + 1];
      std::strcpy(cid_cstr, cid.c_str());

      logger = robotics::logger::Logger(tag_.c_str(), cid_cstr);
    }

    return logger.value();
  }
};

template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
class SharedContext {
  std::shared_ptr<Context<Clock>> ctx;

 public:
  SharedContext(std::weak_ptr<RootContext<Clock>> root, std::string_view tag)
      : ctx(std::make_shared<Context<Clock>>(root, tag)) {}

  SharedContext(std::weak_ptr<RootContext<Clock>> root, std::string_view tag,
                std::string_view parent_path)
      : ctx(std::make_shared<Context<Clock>>(root, tag, parent_path)) {}

  auto Root() -> std::weak_ptr<RootContext<Clock>> { return ctx->Root(); }

  auto GetLoop() -> runtime::Loop<Clock>& { return Root().lock()->GetLoop(); }

  auto ContextId() -> std::string { return ctx->ContextId(); }

  auto Child(std::string tag) -> SharedContext<Clock> {
    auto child = SharedContext<Clock>(Root(), tag, ContextId());
    ctx->AddChild(child);

    return child;
  }

  auto Sleep(std::chrono::milliseconds duration) {
    return runtime::Sleep(GetLoop(), duration);
  }

  auto Logger() -> robotics::logger::Logger& { return ctx->Logger(); }

  auto GetDebugInfo(std::string const& tag) -> debug::DebugInfo<Clock> {
    return debug::DebugInfo<Clock>(*this, ContextId() + "." + tag);
  }

  inline auto AddTask(std::coroutine_handle<> coroutine) -> void {
    Root().lock()->AddTask(coroutine);
  }
};
}  // namespace robobus::context