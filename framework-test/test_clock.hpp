#pragma once

#include <chrono>

class TestClock {
 public:
  using duration = std::chrono::milliseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<TestClock, duration>;

 private:
  using upper_clock = std::chrono::high_resolution_clock;
  static upper_clock::time_point epoch_;

 public:
  static time_point now() {
    auto delta = std::chrono::high_resolution_clock::now() - TestClock::epoch_;
    auto duration = std::chrono::duration_cast<TestClock::duration>(delta);

    return time_point(duration);
  }
};

TestClock::upper_clock::time_point TestClock::epoch_ =
    TestClock::upper_clock::now();

template <>
struct std::chrono::is_clock<TestClock> : std::true_type {};