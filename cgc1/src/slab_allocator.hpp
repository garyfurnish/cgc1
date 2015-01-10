#pragma once
#include <cgc1/posix_slab.hpp>
#include <cgc1/win32_slab.hpp>
#include <cgc1/concurrency.hpp>
#include "object_state.hpp"
namespace cgc1
{
  namespace details
  {
    using slab_allocator_object_t = details::object_state_t;
    static_assert(sizeof(slab_allocator_object_t) == c_alignment, "slab_allocator_object_t too large");
    static_assert(::std::is_pod<slab_allocator_object_t>::value, "slab_allocator_object_t is not POD");
    /**
    This is a thread safe non-reentrant slab allocator.
    It differs from allocator_t in that this allocator is completely in place.
    This is both less memory and runtime efficient, but can be used to back the allocator_t.
    **/
    class slab_allocator_t
    {
    public:
      slab_allocator_t(size_t size, size_t size_hint);
      slab_allocator_t(const slab_allocator_t &) = delete;
      slab_allocator_t(slab_allocator_t &&) = delete;
      uint8_t *begin() const;
      uint8_t *end() const;
      next_iterator<slab_allocator_object_t> _u_object_begin();
      next_iterator<slab_allocator_object_t> _u_object_end();
      next_iterator<slab_allocator_object_t> _u_object_current_end();
      bool _u_empty();
      auto &_mutex() RETURN_CAPABILITY(m_mutex)
      {
        return m_mutex;
      }

      void *_u_split_allocate(slab_allocator_object_t *object, size_t sz);
      void *_u_allocate_raw_at_end(size_t sz);
      void *allocate_raw(size_t sz) REQUIRES(!m_mutex);
      void deallocate_raw(void *v) REQUIRES(!m_mutex);

    private:
      spinlock_t m_mutex;
      slab_t m_slab;
      slab_allocator_object_t *m_end;
    };
  }
}
#ifdef CGC1_INLINES
#include "slab_allocator_impl.hpp"
#endif
