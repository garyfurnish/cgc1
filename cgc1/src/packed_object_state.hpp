#pragma once
#include <cgc1/declarations.hpp>
namespace cgc1
{
  namespace details
  {
    template <size_t Total_Size, size_t Entry_Size, size_t Header_Alignment>
    struct packed_object_state_t {
    public:
      static const constexpr size_t cs_entry_sz = Entry_Size;
      static const constexpr size_t cs_header_alignment = Header_Alignment;
      static const constexpr size_t cs_object_alignment = 32;

      static constexpr size_t real_entry_size()
      {
        return cs_object_alignment > cs_entry_sz ? cs_object_alignment : cs_entry_sz;
      }

      static constexpr size_t num_entries()
      {
        return (Total_Size - cs_header_alignment) / real_entry_size();
      }
      static constexpr size_t num_entries_up()
      {
        return (num_entries() % 256) ? ((num_entries() / 256) + 1) * 256 : num_entries();
      }

      static constexpr size_t num_blocks_needed()
      {
        return num_entries_up() / 256;
      }
      static_assert(cs_header_alignment == num_blocks_needed() * 2 * 8 * 8, "");
      void initialize() noexcept;
      void clear_mark_bits() noexcept;

      auto any_free() const noexcept -> bool;
      auto none_free() const noexcept -> bool;
      auto first_free() const noexcept -> size_t;
      auto any_marked() const noexcept -> bool;
      auto none_marked() const noexcept -> bool;

      auto is_free(size_t i) const noexcept -> bool;
      auto is_marked(size_t i) const noexcept -> bool;

      constexpr auto size() const noexcept -> size_t;
      constexpr auto size_bytes() const noexcept -> size_t;
      auto total_size_bytes() const noexcept -> size_t;
      auto begin() noexcept -> uint8_t *;
      auto end() noexcept -> uint8_t *;

      void set_free(size_t i, bool val) noexcept;
      void set_marked(size_t i) noexcept;

      void *allocate() noexcept;
      bool deallocate(void *v) noexcept;

      void free_unmarked() noexcept;

      ::std::array<uint64_t, num_blocks_needed() * 8> m_free_bits;
      ::std::array<uint64_t, num_blocks_needed() * 8> m_mark_bits;
    };
    static_assert(::std::is_pod<packed_object_state_t<4096 * 4, 64, 128>>::value, "");
  }
}
#include "packed_object_state_impl.hpp"
