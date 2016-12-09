#pragma once
#include "global_kernel_state.hpp"
#include "thread_local_kernel_state_impl.hpp"
namespace cgc1::details
{
  inline auto global_kernel_state_t::gc_allocator() const noexcept -> gc_allocator_t &
  {
    return m_gc_allocator;
  }
  inline auto global_kernel_state_t::_bitmap_allocator() const noexcept -> bitmap_allocator_type &
  {
    return m_bitmap_allocator;
  }
  inline auto global_kernel_state_t::_internal_allocator() const noexcept -> internal_allocator_t &
  {
    return m_cgc_allocator;
  }
  inline auto global_kernel_state_t::_internal_slab_allocator() const noexcept -> internal_slab_allocator_type &
  {
    return m_slab_allocator;
  }
  inline auto global_kernel_state_t::tlks(::std::thread::id id) -> thread_local_kernel_state_t *
  {
    MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_thread_mutex);
    auto it = ::std::find_if(m_threads.begin(), m_threads.end(),
                             [id](thread_local_kernel_state_t *tlks) { return tlks->thread_id() == id; });
    if (it == m_threads.end())
      return nullptr;
    return *it;
  }
  template <typename Container>
  void global_kernel_state_t::_add_freed_in_last_collection(Container &container)
  {
    MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
    m_freed_in_last_collection.insert(m_freed_in_last_collection.end(), container.begin(), container.end());
  }
  template <typename Container>
  void global_kernel_state_t::_add_need_special_finalizing_collection(Container &container)
  {
    MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
    m_need_special_finalizing_collection_sparse.insert(m_need_special_finalizing_collection_sparse.end(), container.begin(),
                                                       container.end());
  }

  inline bool global_kernel_state_t::is_valid_object_state(const gc_sparse_object_state_t *os) const
  {
    // We may assume this because the internal allocator may only grow.
    // At worse this may be falsely negative, but then it is undef behavior race condition.
    MCPPALLOC_CONCURRENCY_LOCK_ASSUME(_internal_allocator()._mutex());
    return ::mcppalloc::sparse::details::is_valid_object_state(os, _internal_allocator().underlying_memory().begin(),
                                                               _internal_allocator()._u_current_end());
  }
  inline auto global_kernel_state_t::_mutex() const -> mutex_type &
  {
    return m_mutex;
  }
  inline auto global_kernel_state_t::root_collection()
  {
    return m_roots;
  }
  inline auto global_kernel_state_t::root_collection() const
  {
    return m_roots;
  }
}
