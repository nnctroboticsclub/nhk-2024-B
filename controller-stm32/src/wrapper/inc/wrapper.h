#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void sleep_ms(int ms);

// re-export
void* malloc(size_t size);
void free(void* ptr);

#ifdef __cplusplus
}
#endif