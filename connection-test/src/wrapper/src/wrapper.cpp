#include "wrapper.h"

#include <mbed.h>
#include <chrono>

#include <robotics/logger/logger.hpp>

static robotics::logger::Logger logger{"direct", "   LOG   "};

void sleep_ms(int ms) {
  using namespace std::chrono_literals;

  ThisThread::sleep_for(ms * 1ms);
}

void __syoch_put_log(const char* line) { logger.Info("%s", line); }