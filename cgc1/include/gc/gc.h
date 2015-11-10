#pragma once
#define CGC1_FAKE_BOEHM
/*
 * Boehm GC compatability header.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include "../cgc1/gc.h"
#include "gc_version.h"
extern CGC1_DLL_PUBLIC unsigned GC_get_version();
#ifdef __cplusplus
}
#endif
