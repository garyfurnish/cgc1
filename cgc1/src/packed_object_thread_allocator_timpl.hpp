#pragma once
namespace cgc1
{
  namespace details
  {
    template <typename Allocator_Policy>
    template <typename Predicate>
    void packed_object_thread_allocator_t<Allocator_Policy>::for_all_state(Predicate &&predicate)
    {
      m_locals.for_all(::std::forward<Predicate>(predicate));
    }
  }
}
