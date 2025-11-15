#pragma once

#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

void* __ffi_malloc(size_t size);
void __ffi_free(void* ptr);

#ifdef __cplusplus
}
#endif
