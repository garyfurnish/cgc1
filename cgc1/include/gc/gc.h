#pragma once
/*
 * Boehm GC compatability header.
 */
#include "../cgc1/gc.h"
#include "gc_version.h"
static inline unsigned GC_get_version()
  {
    return (unsigned)((GC_VERSION_MAJOR<<16) | (GC_VERSION_MINOR<<8));
  }
