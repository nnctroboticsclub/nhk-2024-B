#include "nhk2024b/ffi_mem.hpp"

#include <cstdlib>

void* __ffi_malloc(size_t size) { return malloc(size); }

void __ffi_free(void* ptr) { free(ptr); }