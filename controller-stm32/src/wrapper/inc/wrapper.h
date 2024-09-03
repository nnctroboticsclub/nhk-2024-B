#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void __syoch_sleep_ms(int ms);

void* __syoch_malloc(size_t size, size_t align);
void __syoch_free(void* ptr);

void __syoch_put_log(const char*);

#ifdef __cplusplus
}
#endif