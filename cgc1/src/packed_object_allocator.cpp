#include "bitmap_allocator.hpp"
#include "bitmap_allocator_impl.hpp"
#include "bitmap_thread_allocator.hpp"
#include "global_kernel_state.hpp"
namespace cgc1
{
#ifdef __APPLE__
  template <>
  pthread_key_t
      cgc1::thread_local_pointer_t<cgc1::details::bitmap_thread_allocator_t<details::gc_bitmap_allocator_policy_t>>::s_pkey{0};
#else
  template <>
  thread_local cgc1::thread_local_pointer_t<
      cgc1::details::bitmap_thread_allocator_t<details::gc_bitmap_allocator_policy_t>>::pointer_type
      cgc1::thread_local_pointer_t<cgc1::details::bitmap_thread_allocator_t<details::gc_bitmap_allocator_policy_t>>::s_tlks =
          nullptr;
#endif
  namespace details
  {
    template <>
    thread_local_pointer_t<typename bitmap_allocator_t<gc_bitmap_allocator_policy_t>::thread_allocator_type>
        bitmap_allocator_t<gc_bitmap_allocator_policy_t>::t_thread_allocator{};

    void gc_bitmap_allocator_policy_t::on_allocate(void *addr, size_t sz)
    {
      secure_zero(addr, sz);
    }
    auto gc_bitmap_allocator_policy_t::on_allocation_failure(const packed_allocation_failure_t &failure)
        -> packed_allocation_failure_action_t
    {
      g_gks->force_collect();
      return packed_allocation_failure_action_t{false, failure.m_failures < 5};
    }
  }
}
