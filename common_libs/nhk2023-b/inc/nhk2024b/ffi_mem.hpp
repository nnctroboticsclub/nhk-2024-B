#pragma once

#include <stddef.h>

extern "C" {
void* __ffi_malloc(size_t size);
void __ffi_free(void* ptr);
}