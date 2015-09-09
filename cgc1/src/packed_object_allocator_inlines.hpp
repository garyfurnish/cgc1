#pragma once
namespace cgc1
{
  namespace details
  {
    inline auto packed_object_allocator_t::_mutex() const noexcept -> mutex_type &
    {
      return m_mutex;
    }
    template <typename Predicate>
    void packed_object_allocator_t::for_all_state(Predicate &&predicate)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      for (auto &&thread : m_thread_allocators)
        thread.second->for_all_state(predicate);
      m_globals.for_all(::std::forward<Predicate>(predicate));
    }
  }
}
