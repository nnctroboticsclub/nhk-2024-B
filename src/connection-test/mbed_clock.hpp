#pragma once

#include <mbed.h>

class MbedClock {
 public:
  using duration = std::chrono::duration<float, std::milli>;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<MbedClock, duration>;

 private:
  static mbed::Timer timer_;

 public:
  static void Init() {
    timer_.reset();
    timer_.start();
  }

  static time_point now() {
    auto t = std::chrono::duration_cast<duration>(timer_.elapsed_time());

    return time_point(t);
  }
};

mbed::Timer MbedClock::timer_;

template <>
struct std::chrono::is_clock<MbedClock> : std::true_type {};

template <>
const bool std::chrono::is_clock_v<MbedClock> = true;