#pragma once
#include <cgc1/declarations.hpp>
#include "integer_block.hpp"
namespace cgc1
{
  namespace details
  {

    static constexpr const size_t c_packed_object_alignment = 32;
    struct alignas(c_packed_object_alignment) packed_object_state_info_t {
      size_t m_num_blocks;
      size_t m_data_entry_sz;
      size_t m_size;
      size_t m_header_size;
    };
    static_assert(sizeof(packed_object_state_info_t) == c_packed_object_alignment, "");

    struct alignas(c_packed_object_alignment) packed_object_state_t {
    public:
      static const constexpr size_t cs_header_alignment = 32;
      static const constexpr size_t cs_object_alignment = 32;
      static const constexpr size_t cs_bits_array_multiple = 2;

      using bits_array_type = integer_block_t<8>;

      auto declared_entry_size() const noexcept -> size_t;
      auto real_entry_size() const noexcept -> size_t;
      auto header_size() const noexcept -> size_t;

      void initialize() noexcept;
      void clear_mark_bits() noexcept;

      auto any_free() const noexcept -> bool;
      auto none_free() const noexcept -> bool;
      auto all_free() const noexcept -> bool;
      auto first_free() const noexcept -> size_t;
      auto any_marked() const noexcept -> bool;
      auto none_marked() const noexcept -> bool;
      auto free_popcount() const noexcept -> size_t;

      auto is_free(size_t i) const noexcept -> bool;
      auto is_marked(size_t i) const noexcept -> bool;

      auto size() const noexcept -> size_t;
      void _compute_size() noexcept;
      auto size_bytes() const noexcept -> size_t;
      auto total_size_bytes() const noexcept -> size_t;
      auto begin() noexcept -> uint8_t *;
      auto end() noexcept -> uint8_t *;

      void set_free(size_t i, bool val) noexcept;
      void set_marked(size_t i) noexcept;

      void *allocate() noexcept;
      bool deallocate(void *v) noexcept;

      void free_unmarked() noexcept;

      auto num_blocks() const noexcept -> size_t;
      auto free_bits() noexcept -> bits_array_type *;
      auto free_bits() const noexcept -> const bits_array_type *;
      auto mark_bits() noexcept -> bits_array_type *;
      auto mark_bits() const noexcept -> const bits_array_type *;
      packed_object_state_info_t m_info;
    };
    static_assert(::std::is_pod<packed_object_state_t>::value, "");
    static_assert(sizeof(packed_object_state_t) == c_packed_object_alignment, "");
  }
}
#ifdef CGC1_INLINES
#include "packed_object_state_impl.hpp"
#endif
