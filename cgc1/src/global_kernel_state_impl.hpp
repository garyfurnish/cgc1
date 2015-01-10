#pragma once
#include "global_kernel_state.hpp"
#include "thread_local_kernel_state_impl.hpp"
namespace cgc1
{
  namespace details
  {
    extern global_kernel_state_t g_gks;
    inline auto global_kernel_state_t::gc_allocator() const -> gc_allocator_t &
    {
      return m_gc_allocator;
    }
    inline auto global_kernel_state_t::_internal_allocator() const -> internal_allocator_t &
    {
      return m_cgc_allocator;
    }
    inline auto global_kernel_state_t::_internal_slab_allocator() const -> slab_allocator_t &
    {
      return m_slab_allocator;
    }
    inline auto global_kernel_state_t::tlks(::std::thread::id id) -> thread_local_kernel_state_t *
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_thread_mutex);
      auto it = ::std::find_if(m_threads.begin(), m_threads.end(),
                               [id](thread_local_kernel_state_t *tlks) { return tlks->thread_id() == id; });
      if (it == m_threads.end())
        return nullptr;
      return *it;
    }
    template <typename Container>
    inline void global_kernel_state_t::_add_freed_in_last_collection(Container &container)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      m_freed_in_last_collection.insert(m_freed_in_last_collection.end(), container.begin(), container.end());
    }
    inline bool global_kernel_state_t::is_valid_object_state(const object_state_t *os) const
    {
      return details::is_valid_object_state(os, _internal_allocator()._u_begin(), _internal_allocator()._u_current_end());
    }
    inline mutex_t &global_kernel_state_t::_mutex() const RETURN_CAPABILITY(m_mutex)
    {
      return m_mutex;
    }
  }
}
