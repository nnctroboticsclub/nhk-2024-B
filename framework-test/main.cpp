#include <chrono>
#include <iostream>

#include "framework/context.hpp"
#include "framework/base_coroutine.hpp"

#include "test_clock.hpp"
#include "measurement.hpp"

#include "framework/debug_node.hpp"

using Clock = TestClock;

using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;

class Label {
 public:
  void RenderTo(render::BaseRenderer& renderer) {
    renderer.RenderHTML("<label>Test</label>");
  }
};

class SimpleRenderer : public render::BaseRenderer {
 public:
  ~SimpleRenderer() override = default;

  void RenderHTML(std::string const& html) override {
    std::cout << "$HTML:" << html << std::endl;
  }

  void RenderComponent(render::ComponentByteCode const& component) override {
    std::cout << "RenderComponent" << std::endl;
  }
};

Coroutine<void> Task(SharedContext<Clock> ctx,
                     std::chrono::milliseconds delay) {
  auto logger = ctx.Logger();

  logger.RenameTag("   Test  ");

  auto renderer = SimpleRenderer{};
  DebugNode<Label> node(1, "Test", Label{});
  node.Render(renderer);

  co_await ctx.Sleep(delay);

  while (true) {
    const auto now = ctx.GetLoop().time.Now().time_since_epoch();
    const auto now_s =
        std::chrono::duration_cast<std::chrono::seconds>(now).count();

    logger.Info("%10d", now_s);
    co_await ctx.Sleep(1s);
  }

  co_return;
}

void LaunchLoopTask(SharedContext<Clock> ctx) {
  ctx.AddTask(Task(ctx, 1000ms / 1).handle);
  ctx.AddTask(Task(ctx, 1000ms / 2).handle);
  ctx.AddTask(Task(ctx, 1000ms / 3).handle);
}

int main() {
  robotics::system::SleepFor(20ms);
  robotics::logger::core::Init();

  robotics::logger::SuppressLogger("loop.robobus");

  auto ctx = SharedRootContext<Clock>();
  // ctx.GetLoop().LaunchDebugThread();

  LaunchLoopTask(ctx.Child("Loop"));

  ctx.Run();

  return 0;
}
