#pragma once
#include <atomic>
namespace cgc1
{
  namespace details
  {
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    void packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::initialize() noexcept
    {
      ::std::fill(m_free_bits.begin(), m_free_bits.end(), ::std::numeric_limits<uint64_t>::max());
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    void packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::clear_mark_bits() noexcept
    {
      ::std::fill(m_mark_bits.begin(), m_mark_bits.end(), 0);
    }

    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::any_free() const noexcept -> bool
    {
      for (auto &&it : m_free_bits)
        if (it)
          return true;
      return false;
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::none_free() const noexcept -> bool
    {
      for (auto &&it : m_free_bits)
        if (it)
          return false;
      return true;
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::first_free() const noexcept -> size_t
    {
      for (size_t i = 0; i < m_free_bits.size(); ++i) {
        const uint64_t &it = m_free_bits[i];
        auto first = static_cast<size_t>(__builtin_ffsll(static_cast<long long>(it)));
        if (first)
          return (64 * i) + (first - 1);
      }
      return ::std::numeric_limits<size_t>::max();
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::any_marked() const noexcept -> bool
    {
      for (auto &&it : m_mark_bits)
        if (it)
          return true;
      return false;
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::none_marked() const noexcept -> bool
    {
      for (auto &&it : m_mark_bits)
        if (it)
          return false;
      return true;
    }

    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    void packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::set_free(size_t i, bool val) noexcept
    {
      auto pos = i / 64;
      auto sub_pos = i - (pos * 64);
      m_free_bits[pos] = (m_free_bits[pos] & (~(1ll << sub_pos))) | (static_cast<size_t>(val) << sub_pos);
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    void packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::set_marked(size_t i) noexcept
    {
      auto pos = i / 64;
      auto sub_pos = i - (pos * 64);
      auto &bits = unsafe_reference_cast<::std::atomic<uint64_t>>(m_mark_bits[pos]);
      bool success = false;
      while (!success) {
        auto cur = bits.load(::std::memory_order_relaxed);
        auto new_val = (cur & (~(1ll << sub_pos))) | (1ll << sub_pos);
        success = bits.compare_exchange_weak(cur, new_val, ::std::memory_order_relaxed);
      }
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::is_free(size_t i) const noexcept -> bool
    {
      auto pos = i / 64;
      auto sub_pos = i - (pos * 64);
      return (m_free_bits[pos] & (1ll << sub_pos)) > sub_pos;
    }
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    auto packed_object_state_t<Total_Size, Entry_Size, Header_Alignment>::is_marked(size_t i) const noexcept -> bool
    {
      auto pos = i / 64;
      auto sub_pos = i - (pos * 64);
      auto ret = (m_mark_bits[pos] & (1ll << sub_pos)) > (sub_pos);
      return ret;
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
      for (size_t i = 0; i < m_free_bits.size(); ++i) {
        m_free_bits[i] |= (~m_mark_bits[i]);
      }
    }
  }
}
