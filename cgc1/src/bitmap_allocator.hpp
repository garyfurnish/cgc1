#pragma once
namespace cgc1
{
  class gc_bitmap_allocator_policy_t
  {
  public:
    void on_allocate(void *addr, size_t sz);
    auto on_allocation_failure(const packed_allocation_failure_t &action) -> packed_allocation_failure_action_t;
  };
}
