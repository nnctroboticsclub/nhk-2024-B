#include "wrapper.h"

#include <mbed.h>
#include <chrono>

#include <robotics/logger/logger.hpp>

static robotics::logger::Logger logger{"direct", "   LOG   "};

void __syoch_sleep_ms(int ms) {
  using namespace std::chrono_literals;

  ThisThread::sleep_for(ms * 1ms);
}

void* __syoch_malloc(size_t size, size_t align) {
  auto ptr = malloc(size);
  if (0) logger.Trace("malloc(%d, %d) --> %p", size, align, ptr);

  return ptr;
}

void __syoch_free(void* ptr) {
  if (0)
    logger.Trace("free(%p) --> (%02X%02X%02X%02X)", ptr, ((uint8_t*)ptr)[0],
                 ((uint8_t*)ptr)[1], ((uint8_t*)ptr)[2], ((uint8_t*)ptr)[3]);

  free(ptr);
}

void __syoch_put_log(const char* line) { logger.Info("%s", line); }