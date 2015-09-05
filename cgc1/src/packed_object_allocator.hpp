#pragma once
#include "packed_object_thread_allocator.hpp"
#include "slab_allocator.hpp"
namespace cgc1
{
  namespace details
  {

    class packed_object_allocator_t
    {
    public:
      using mutex_type = spinlock_t;

      packed_object_allocator_t(size_t size, size_t size_hint);
      packed_object_allocator_t(const packed_object_allocator_t &) = delete;
      packed_object_allocator_t(packed_object_allocator_t &&) noexcept = delete;
      packed_object_allocator_t &operator=(const packed_object_allocator_t &) = delete;
      packed_object_allocator_t &operator=(packed_object_allocator_t &&) noexcept = delete;
      ~packed_object_allocator_t();
      REQUIRES(!m_mutex) auto _get_memory() noexcept -> packed_object_state_t *;
      void _u_to_global(size_t id, packed_object_state_t *state) noexcept REQUIRES(m_mutex);
      void _u_to_free(void *v) noexcept REQUIRES(m_mutex);

      RETURN_CAPABILITY(m_mutex) auto _mutex() const noexcept -> mutex_type &;

    private:
      slab_allocator_t m_slab;
      packed_object_package_t m_globals GUARDED_BY(m_mutex);
      cgc_internal_vector_t<void *> m_free_globals GUARDED_BY(m_mutex);
      mutable mutex_type m_mutex;
    };
  }
}
#include "packed_object_allocator_impl.hpp"
