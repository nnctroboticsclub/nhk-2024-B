#pragma once

#include <memory>

#include "sleep.hpp"
#include "debug_adapter.hpp"

template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
class SharedContext;

/// @brief コルーチンベースプログラムで用いるコンテキスト
template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
struct RootContext {
 private:
  std::vector<SharedContext<Clock>> child_contexts_;
  Loop<Clock> loop_{};

  std::optional<std::shared_ptr<DebugAdapter>> debug_adapter_;

 public:
  [[noreturn]]
  auto Run() -> void {
    loop_.Run();
  }

  auto GetLoop() -> Loop<Clock>& { return loop_; }

  auto SetDebugAdapter(std::shared_ptr<DebugAdapter> adapter) -> void {
    debug_adapter_ = adapter;
  }

  auto GetDebugAdapter() -> std::optional<std::shared_ptr<DebugAdapter>> {
    return debug_adapter_;
  }

  auto AddChild(SharedContext<Clock> sub_context) -> void {
    child_contexts_.emplace_back(sub_context);
  }

  inline auto AddTask(std::coroutine_handle<> coroutine) -> void {
    loop_.AddTask(coroutine);
  }
};

template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
class SharedRootContext {
  using RootCtx = RootContext<Clock>;

  std::shared_ptr<RootCtx> root = std::make_shared<RootCtx>();

 public:
  auto Root() -> std::shared_ptr<RootCtx> { return root; }

  [[noreturn]]
  auto Run() -> void {
    root->Run();
  }

  auto GetLoop() -> Loop<Clock>& { return root->GetLoop(); }

  auto SetDebugAdapter(std::shared_ptr<DebugAdapter> adapter) -> void {
    root->SetDebugAdapter(adapter);
  }

  auto GetDebugAdapter() -> std::optional<std::shared_ptr<DebugAdapter>> {
    return root->DebugAdapter();
  }

  auto Child(std::string tag) -> SharedContext<Clock> {
    auto ctx = SharedContext<Clock>(root, tag);
    root->AddChild(ctx);
    return ctx;
  }

  inline auto AddTask(std::coroutine_handle<> coroutine) -> void {
    root->AddTask(coroutine);
  }
};