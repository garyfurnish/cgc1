#pragma once
#include <mcppalloc/mcppalloc_utils/declarations.hpp>
namespace cgc1
{
  template <size_t bytes = 5000>
  extern void clean_stack(size_t, size_t, size_t, size_t, size_t);
}
