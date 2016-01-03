#pragma once
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {
      inline bitmap_type_info_t::bitmap_type_info_t(type_id_t type_id, uint8_t num_user_bits)
          : m_type_id(type_id), m_num_user_bits(num_user_bits)
      {
      }
      inline auto bitmap_type_info_t::type_id() const noexcept -> type_id_t
      {
        return m_type_id;
      }
      inline void bitmap_type_info_t::set_type_id(type_id_t type_id) noexcept
      {
        m_type_id = type_id;
      }
      inline auto bitmap_type_info_t::num_user_bits() const noexcept -> uint8_t
      {
        return m_num_user_bits;
      }
      inline void bitmap_type_info_t::set_num_user_bits(uint8_t num_user_bits) noexcept
      {
        m_num_user_bits = num_user_bits;
      }
    }
  }
}
