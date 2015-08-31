#pragma once
#include "packed_object_thread_allocator.hpp"
namespace cgc1
{
  namespace details
  {

    class packed_object_allocator_t
    {
    public:
    private:
      packed_object_package_t m_globals GUARDED_BY(m_mutex);
      packed_object_package_t m_free_globals GUARDED_BY(m_mutex);
      spinlock_t m_mutex;
    };
  }
}
#include "packed_object_allocator_impl.hpp"
