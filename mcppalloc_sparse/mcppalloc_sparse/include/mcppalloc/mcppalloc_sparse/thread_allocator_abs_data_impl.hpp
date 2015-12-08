#pragma once
namespace mcppalloc
{
  namespace sparse
  {
    namespace details
    {
      inline thread_allocator_abs_data_t::thread_allocator_abs_data_t(uint32_t allocator_multiple,
                                                                      uint32_t max_blocks_before_recycle)
          : m_allocator_multiple(allocator_multiple), m_max_blocks_before_recycle(max_blocks_before_recycle)
      {
      }
      inline auto thread_allocator_abs_data_t::allocator_multiple() const noexcept -> uint32_t
      {
        return m_allocator_multiple;
      }
      inline auto thread_allocator_abs_data_t::max_blocks_before_recycle() const noexcept -> uint32_t
      {
        return m_max_blocks_before_recycle;
      }
      inline void thread_allocator_abs_data_t::set_allocator_multiple(uint32_t multiple)
      {
        m_allocator_multiple = multiple;
      }
      inline void thread_allocator_abs_data_t::set_max_blocks_before_recycle(uint32_t blocks)
      {
        m_max_blocks_before_recycle = blocks;
      }
    }
  }
}
