#pragma once
#include <atomic>
#include <iostream>
namespace cgc1
{
  namespace details
  {

    ALWAYS_INLINE inline auto packed_object_state_t::real_entry_size(const packed_object_state_info_t &state) const noexcept
        -> size_t
    {
      return cs_object_alignment > state.m_data_entry_sz ? cs_object_alignment : state.m_data_entry_sz;
    }
    ALWAYS_INLINE inline auto packed_object_state_t::header_size(const packed_object_state_info_t &state) const noexcept -> size_t
    {
      auto blocks = state.m_num_blocks;
      auto unaligned = sizeof(bits_array_type) * blocks * cs_bits_array_multiple;
      return align(unaligned, cs_header_alignment);
    }
    void packed_object_state_t::initialize(const packed_object_state_info_t &state) noexcept
    {
      for (size_t i = 0; i < num_blocks(state); ++i)
        free_bits(state)[i].fill(::std::numeric_limits<uint64_t>::max());
    }
    void packed_object_state_t::clear_mark_bits(const packed_object_state_info_t &state) noexcept
    {
      for (size_t i = 0; i < num_blocks(state); ++i)
        mark_bits(state)[i].fill(0);
    }

    auto packed_object_state_t::any_free(const packed_object_state_info_t &state) const noexcept -> bool
    {
      for (size_t i = 0; i < num_blocks(state); ++i) {
        auto &&it = free_bits(state)[i];
        if (it.any_set())
          return true;
      }
      return false;
    }
    auto packed_object_state_t::none_free(const packed_object_state_info_t &state) const noexcept -> bool
    {
      for (size_t i = 0; i < num_blocks(state); ++i) {
        auto &&it = free_bits(state)[i];
        if (it.any_set())
          return false;
      }
      return true;
    }
    auto packed_object_state_t::first_free(const packed_object_state_info_t &state) const noexcept -> size_t
    {
      for (size_t i = 0; i < num_blocks(state); ++i) {
        auto ret = free_bits(state)[i].first_set();
        if (ret != ::std::numeric_limits<size_t>::max())
          return i * 64 + ret;
      }
      return ::std::numeric_limits<size_t>::max();
    }
    auto packed_object_state_t::any_marked(const packed_object_state_info_t &state) const noexcept -> bool
    {
      for (size_t i = 0; i < num_blocks(state); ++i) {
        auto &&it = mark_bits(state)[i];
        if (it.any_set())
          return true;
      }
      return false;
    }
    auto packed_object_state_t::none_marked(const packed_object_state_info_t &state) const noexcept -> bool
    {
      for (size_t i = 0; i < num_blocks(state); ++i) {
        auto &&it = mark_bits(state)[i];
        if (it.any_set())
          return false;
      }
      return true;
    }

    void packed_object_state_t::set_free(const packed_object_state_info_t &state, size_t i, bool val) noexcept
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      free_bits(state)[pos].set_bit(sub_pos, val);
    }
    void packed_object_state_t::set_marked(const packed_object_state_info_t &state, size_t i) noexcept
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      mark_bits(state)[pos].set_bit_atomic(sub_pos, true, ::std::memory_order_relaxed);
    }
    auto packed_object_state_t::is_free(const packed_object_state_info_t &state, size_t i) const noexcept -> bool
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      return free_bits(state)[pos].get_bit(sub_pos);
    }
    auto packed_object_state_t::is_marked(const packed_object_state_info_t &state, size_t i) const noexcept -> bool
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      return mark_bits(state)[pos].get_bit(sub_pos);
    }
    auto packed_object_state_t::size(const packed_object_state_info_t &state) const noexcept -> size_t
    {
      auto blocks = state.m_num_blocks;
      auto hdr_sz = header_size(state);
      auto data_entry_sz = state.m_data_entry_sz;
      auto data_sz = blocks * data_entry_sz * bits_array_type::size_in_bits() - hdr_sz;
      auto num_data = data_sz / (real_entry_size(state));
      return num_data;
    }
    auto packed_object_state_t::size_bytes(const packed_object_state_info_t &state) const noexcept -> size_t
    {
      return size(state) * real_entry_size(state);
    }

    auto packed_object_state_t::begin(const packed_object_state_info_t &state) noexcept -> uint8_t *
    {
      return reinterpret_cast<uint8_t *>(this) + header_size(state);
    }
    auto packed_object_state_t::end(const packed_object_state_info_t &state) noexcept -> uint8_t *
    {
      return begin(state) + size_bytes(state);
    }

    void *packed_object_state_t::allocate(const packed_object_state_info_t &state) noexcept
    {
      auto i = first_free(state);
      if (i >= size(state))
        return nullptr;
      if (i == ::std::numeric_limits<size_t>::max())
        return nullptr;
      set_free(state, i, false);
      return begin(state) + real_entry_size(state) * i;
    }
    bool packed_object_state_t::deallocate(const packed_object_state_info_t &state, void *vv) noexcept
    {
      auto v = reinterpret_cast<uint8_t *>(vv);
      if (unlikely(v < begin(state) || v > end(state)))
        return false;
      size_t byte_diff = static_cast<size_t>(v - begin(state));
      if (unlikely(byte_diff % real_entry_size(state)))
        return false;
      auto i = byte_diff / real_entry_size(state);
      set_free(state, i, true);
      return true;
    }

    void packed_object_state_t::free_unmarked(const packed_object_state_info_t &state) noexcept
    {
      for (size_t i = 0; i < num_blocks(state); ++i)
        free_bits(state)[i] |= ~mark_bits(state)[i];
    }
    auto packed_object_state_t::num_blocks(const packed_object_state_info_t &state) const noexcept -> size_t
    {
      return state.m_num_blocks;
    }
    auto packed_object_state_t::free_bits(const packed_object_state_info_t &) noexcept -> bits_array_type *
    {
      return unsafe_cast<bits_array_type>(this);
    }
    auto packed_object_state_t::free_bits(const packed_object_state_info_t &) const noexcept -> const bits_array_type *
    {
      return unsafe_cast<bits_array_type>(this);
    }

    auto packed_object_state_t::mark_bits(const packed_object_state_info_t &state) noexcept -> bits_array_type *
    {
      return free_bits(state) + num_blocks(state);
    }
    auto packed_object_state_t::mark_bits(const packed_object_state_info_t &state) const noexcept -> const bits_array_type *
    {
      return free_bits(state) + num_blocks(state);
    }
  }
}
