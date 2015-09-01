#pragma once
#include <atomic>
namespace cgc1
{
  namespace details
  {
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    void packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::initialize() noexcept
    {
      for (auto &&it : m_free_bits)
        it.fill(::std::numeric_limits<uint64_t>::max());
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    void packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::clear_mark_bits() noexcept
    {
      for (auto &&it : m_mark_bits)
        it.clear();
    }

    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::any_free() const noexcept -> bool
    {
      for (auto &&it : m_free_bits)
        if (it.any_set())
          return true;
      return false;
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::none_free() const noexcept -> bool
    {
      for (auto &&it : m_free_bits)
        if (it.any_set())
          return false;
      return true;
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::first_free() const noexcept -> size_t
    {
      for (size_t i = 0; i < m_free_bits.size(); ++i) {
        auto ret = m_free_bits[i].first_set();
        if (ret != ::std::numeric_limits<size_t>::max())
          return i * 64 + ret;
      }
      return ::std::numeric_limits<size_t>::max();
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::any_marked() const noexcept -> bool
    {
      for (auto &&it : m_mark_bits)
        if (it.any_set())
          return true;
      return false;
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::none_marked() const noexcept -> bool
    {
      for (auto &&it : m_mark_bits)
        if (it.any_set())
          return false;
      return true;
    }

    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    void packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::set_free(size_t i, bool val) noexcept
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      m_free_bits[pos].set_bit(sub_pos, val);
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    void packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::set_marked(size_t i) noexcept
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      m_mark_bits[pos].set_bit_atomic(sub_pos, true, ::std::memory_order_relaxed);
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::is_free(size_t i) const noexcept -> bool
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      return m_free_bits[pos].get_bit(sub_pos);
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::is_marked(size_t i) const noexcept -> bool
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      return m_mark_bits[pos].get_bit(sub_pos);
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    constexpr auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::size() const noexcept -> size_t
    {
      return num_entries();
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    constexpr auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::size_bytes() const noexcept -> size_t
    {
      return size() * real_entry_size();
    }

    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::begin() noexcept -> uint8_t *
    {
      return reinterpret_cast<uint8_t *>(this) + align(sizeof(this), cs_header_alignment);
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::end() noexcept -> uint8_t *
    {
      return begin() + size_bytes();
    }

    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    void *packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::allocate() noexcept
    {
      auto i = first_free();
      if (i >= num_entries())
        return nullptr;
      if (i == ::std::numeric_limits<size_t>::max())
        return nullptr;
      set_free(i, false);
      return begin() + real_entry_size() * i;
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    bool packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::deallocate(void *vv) noexcept
    {
      auto v = reinterpret_cast<uint8_t *>(vv);
      if (unlikely(v < begin() || v > end()))
        return false;
      size_t byte_diff = static_cast<size_t>(v - begin());
      if (unlikely(byte_diff % real_entry_size()))
        return false;
      auto i = byte_diff / real_entry_size();
      set_free(i, true);
      return true;
    }

    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    void packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::free_unmarked() noexcept
    {
      for (size_t i = 0; i < m_free_bits.size(); ++i)
        m_free_bits[i] |= ~m_mark_bits[i];
    }
  }
}
