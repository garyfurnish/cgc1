#pragma once
#include "gc_user_data.hpp"
namespace cgc1
{
  namespace details
  {
    inline auto gc_user_data_t::finalizer_ref() noexcept -> finalizer_type &
    {
      return m_finalizer;
    }
    inline auto gc_user_data_t::finalizer_ref() const noexcept -> const finalizer_type &
    {
      return m_finalizer;
    }
  }
}
