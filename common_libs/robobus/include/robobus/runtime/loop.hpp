#pragma once

#include <coroutine>
#include <chrono>
#include <list>

#include <logger/logger.hpp>
#include <robotics/thread/thread.hpp>

#include "time_context.hpp"

namespace robobus::runtime {
/// @brief コルーチンベースプログラムで用いるコンテキスト
template <typename Clock>
  requires std::chrono::is_clock_v<Clock>
class Loop {
  static inline robotics::logger::Logger logger{"loop.robobus", "Loop "};
  std::list<std::coroutine_handle<>> coroutines_;

  std::list<std::pair<typename Clock::time_point, std::coroutine_handle<>>>
      resume_list_;

 public:
  TimeContext<Clock> time;

 private:
  void ProcessResumeList() {
    auto minimum_grace = Clock::duration::max();
    const auto now = time.Now();

    for (auto &&[c_time, coro] : resume_list_) {
      auto grace = c_time - now;

      if (0 < grace.count() && grace < minimum_grace) {
        minimum_grace = grace;
      }

      if (c_time <= now) {
        logger.Info("Resume at %p (requested as resume in %d)", coro.address(),
                    c_time.time_since_epoch().count());
        coro.resume();

        continue;
      }
    }

    resume_list_.remove_if(
        [this, now](auto const &pair) { return pair.first <= now; });

    if (minimum_grace != Clock::duration::max() &&
        minimum_grace > std::chrono::milliseconds(100)) {
      logger.Debug("Sleeping for %d", minimum_grace.count());
      robotics::system::SleepFor(
          std::chrono::duration_cast<std::chrono::milliseconds>(minimum_grace));
    }
  }

 public:
  Loop(Loop const &) = delete;
  Loop &operator=(Loop const &) = delete;

  Loop() = default;

  //* Loop traits
  void AddTask(std::coroutine_handle<> coroutine) {
    coroutines_.push_back(coroutine);
  }

  void RequestResumeAt(typename Clock::time_point time_point,
                       std::coroutine_handle<> coroutine) {
    auto now = time.Now().time_since_epoch();
    auto delta = time_point - time.Started();

    logger.Info("Requested resume at %p in %d (now %d)", coroutine.address(),
                delta.count(), now.count());
    resume_list_.push_back({time_point, coroutine});
  }

  //* Root context
  [[noreturn]]
  void Run() {
    logger.Trace("Starting main loop");
    while (true) {
      for (auto const &coroutine : coroutines_) {
        if (coroutine.done()) {
          logger.Info("Remove the %p", coroutine.address());
          coroutines_.remove(coroutine);
        }
      }

      time.Tick();
      ProcessResumeList();
    }
  }

  void LaunchDebugThread() {
    using namespace std::chrono_literals;
    static robotics::logger::Logger logger{"debug.loop.robobus",
                                           "Loop\x1b[32mDebug\x1b[m"};

    robotics::system::Thread thread;
    thread.SetThreadName("Loop-Debug");
    thread.Start([this]() {
      while (true) {
        logger.Info("Elapsed: %d, Now: %d, resume_list: %d entries",
                    time.ElapsedTime().count(),
                    time.Now().time_since_epoch().count(), resume_list_.size());

        robotics::system::SleepFor(1s);
      }
    });
  }
};
}  // namespace robobus::runtime