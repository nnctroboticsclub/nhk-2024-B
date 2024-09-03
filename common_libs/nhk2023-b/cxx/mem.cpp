#include "nhk2024b/ffi_mem.hpp"

#include <cstdlib>
#include <stdexcept>
#include <unordered_set>

#include <robotics/logger/logger.hpp>
#include <robotics/platform/thread.hpp>

static robotics::logger::Logger logger{"system.ffi", "FFISystem"};

static std::unordered_set<uint32_t> allocated_pointer;

void* __ffi_malloc(size_t size) {
  auto ptr = malloc(size);
  logger.Info("malloc(%d) => %p", size, ptr);

  allocated_pointer.emplace((uint32_t)ptr);

  return ptr;
}

void __ffi_free(void* ptr) {
  if (allocated_pointer.find((uint32_t)ptr) == allocated_pointer.end()) {
    logger.Error("Double free detected on %p\n", ptr);

    while(1) {
      using namespace std::chrono_literals;
      robotics::system::SleepFor(1s);
    }
  }
  allocated_pointer.erase((uint32_t)ptr);

  logger.Info("free(%p)", ptr);
  free(ptr);
}