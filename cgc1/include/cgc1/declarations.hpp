#pragma once
#include <mcpputil/mcpputil/declarations.hpp>
namespace cgc1
{
  static constexpr const bool c_gc_verbose_track = false;
  template <size_t bytes = 5000>
  extern void clean_stack(size_t, size_t, size_t, size_t, size_t);

  extern template void clean_stack<5000>(size_t, size_t, size_t, size_t, size_t);
  namespace details
  {
    class global_kernel_state_t;
  }
}
