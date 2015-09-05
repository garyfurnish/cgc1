#pragma once
namespace cgc1
{
  namespace details
  {
    inline auto packed_object_allocator_t::_mutex() const noexcept -> mutex_type &
    {
      return m_mutex;
    }
  }
}
