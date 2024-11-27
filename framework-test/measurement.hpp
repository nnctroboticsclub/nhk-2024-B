#pragma once

#include "framework/context.hpp"
#include "framework/base_coroutine.hpp"

#include <chrono>

template <typename Clock>
class Measurement {
  struct MeasurementData {
    float tps;
    float avg_tps;
    int delta_ticks;
  };

 public:
  Measurement(SharedContext<Clock> ctx) : ctx_(ctx) {
    ctx.AddTask(Measure().handle);
    ctx.AddTask(Countup().handle);
  }

  Coroutine<void> Countup() {
    using namespace std::chrono_literals;

    while (true) {
      data_->delta_ticks += 1;
      co_await ctx_.Sleep(1ms);
    }
  }

  Coroutine<void> Measure() {
    using namespace std::chrono_literals;
    auto logger = ctx_.Logger();

    auto begin_time = ctx_.GetLoop().time.Now();

    while (true) {
      auto delta_time = ctx_.GetLoop().time.Now() - begin_time;
      begin_time = ctx_.GetLoop().time.Now();

      data_->tps = data_->delta_ticks /
                   (delta_time.count() == 0 ? 1 : delta_time.count()) * 1000;
      data_->avg_tps = (data_->avg_tps + data_->tps) / 2;
      data_->delta_ticks = 0;

      logger.Info("TPS: %f, avg TPS: %f", data_->tps, data_->avg_tps);

      co_await ctx_.Sleep(1000ms);
    }
  }

 private:
  SharedContext<Clock> ctx_;
  std::shared_ptr<MeasurementData> data_ = std::make_shared<MeasurementData>();
};