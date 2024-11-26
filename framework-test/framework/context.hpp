#pragma once

#include "root_context.hpp"

#include <optional>
#include <cstring>

#include <logger/logger.hpp>

template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
class SharedContext;

/// @brief コルーチンベースプログラムで用いるコンテキスト
template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
struct Context {
 private:
  std::string tag_;

  std::vector<SharedContext<Clock>> child_contexts_;
  std::weak_ptr<Context<Clock>> parent_context;

  std::weak_ptr<RootContext<Clock>> root_ctx;

  std::optional<robotics::logger::Logger> logger;

 public:
  explicit Context(std::weak_ptr<RootContext<Clock>> root,
                   std::weak_ptr<Context<Clock>> parent, std::string const& tag)
      : root_ctx(root), parent_context(parent), tag_(tag) {}

  auto Root() -> std::weak_ptr<RootContext<Clock>> { return root_ctx; }

  auto AddChild(SharedContext<Clock> sub_context) {
    child_contexts_.emplace_back(sub_context);
  }

  auto Parent() -> std::weak_ptr<Context<Clock>> {
    return parent_context.lock();
  }

  auto ContextId() -> std::string {
    std::string context_id = "";
    if (auto parent = Parent().lock()) {
      context_id = parent->ContextId() + ".";
    } else {
      context_id = "";
    }

    context_id += tag_;

    return context_id;
  }

  inline auto AddTask(std::coroutine_handle<> coroutine) -> void {
    Root().loop.AddTask(coroutine);
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
  SharedContext(std::weak_ptr<RootContext<Clock>> root,
                std::weak_ptr<Context<Clock>> parent, std::string tag)
      : ctx(std::make_shared<Context<Clock>>(root, parent, tag)) {}

  auto Root() -> std::weak_ptr<RootContext<Clock>> { return ctx->Root(); }

  auto Parent() -> std::weak_ptr<Context<Clock>> { return ctx->Parent(); }

  auto GetLoop() -> Loop<Clock>& { return Root().lock()->GetLoop(); }

  auto ContextId() -> std::string { return ctx->ContextId(); }

  auto Child(std::string tag) -> SharedContext<Clock> {
    auto child = SharedContext<Clock>(Root(), ctx, tag);
    ctx->AddChild(child);

    return child;
  }

  auto Sleep(std::chrono::milliseconds duration) {
    return ::Sleep(GetLoop(), duration);
  }

  auto Logger() -> robotics::logger::Logger& { return ctx->Logger(); }

  inline auto AddTask(std::coroutine_handle<> coroutine) -> void {
    Root().lock()->AddTask(coroutine);
  }
};