#include "wrapper.h"

#include <mbed.h>
#include <chrono>

void sleep_ms(int ms) {
  using namespace std::chrono_literals;

  ThisThread::sleep_for(ms * 1ms);
}