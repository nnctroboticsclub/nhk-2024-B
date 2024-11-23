#pragma once

#include <chrono>

template <typename ClockT>
  requires std::chrono::is_clock_v<ClockT>
class TimeContext {
  using time_point = ClockT::time_point;
  using duration = ClockT::duration;

  time_point start_;
  time_point now_;
  time_point last_;
  duration delta_;

 public:
  TimeContext() { Reset(); }

  auto Reset() -> void {
    start_ = ClockT::now();

    now_ = start_;
    last_ = start_;
    delta_ = ClockT::duration::zero();
  }

  auto Tick() -> void {
    last_ = now_;
    now_ = ClockT::now();
    delta_ = now_ - last_;
  }

  auto ElapsedTime() const -> duration { return now_ - start_; }

  auto DeltaTime() const -> duration { return delta_; }

  auto Started() const -> time_point { return start_; }

  auto Now() const -> time_point { return now_; }
};
