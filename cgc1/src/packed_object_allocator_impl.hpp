#pragma once
namespace cgc1
{
  namespace details
  {
    CGC1_OPT_INLINE auto packed_object_allocator_t::begin() const noexcept -> uint8_t *
    {
      return m_slab.begin();
    }
    CGC1_OPT_INLINE auto packed_object_allocator_t::end() const noexcept -> uint8_t *
    {
      return m_slab.end();
    }
    CGC1_OPT_INLINE auto packed_object_allocator_t::_slab() const noexcept -> slab_allocator_t &
    {
      return m_slab;
    }
  }
}
