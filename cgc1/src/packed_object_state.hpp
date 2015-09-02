#pragma once
#include <cgc1/declarations.hpp>
#include "integer_block.hpp"
namespace cgc1
{
  namespace details
  {

    struct packed_object_state_info_t {
      size_t m_num_blocks;
      size_t m_data_entry_sz;
    };

    struct alignas(32) packed_object_state_t {
    public:
      static const constexpr size_t cs_header_alignment = 32;
      static const constexpr size_t cs_object_alignment = 32;
      static const constexpr size_t cs_bits_array_multiple = 2;

      using bits_array_type = integer_block_t<8>;

      auto real_entry_size(const packed_object_state_info_t &) const noexcept -> size_t;
      auto header_size(const packed_object_state_info_t &) const noexcept -> size_t;

      void initialize(const packed_object_state_info_t &) noexcept;
      void clear_mark_bits(const packed_object_state_info_t &) noexcept;

      auto any_free(const packed_object_state_info_t &) const noexcept -> bool;
      auto none_free(const packed_object_state_info_t &) const noexcept -> bool;
      auto first_free(const packed_object_state_info_t &) const noexcept -> size_t;
      auto any_marked(const packed_object_state_info_t &) const noexcept -> bool;
      auto none_marked(const packed_object_state_info_t &) const noexcept -> bool;

      auto is_free(const packed_object_state_info_t &, size_t i) const noexcept -> bool;
      auto is_marked(const packed_object_state_info_t &, size_t i) const noexcept -> bool;

      auto size(const packed_object_state_info_t &) const noexcept -> size_t;
      auto size_bytes(const packed_object_state_info_t &) const noexcept -> size_t;
      auto total_size_bytes(const packed_object_state_info_t &) const noexcept -> size_t;
      auto begin(const packed_object_state_info_t &) noexcept -> uint8_t *;
      auto end(const packed_object_state_info_t &) noexcept -> uint8_t *;

      void set_free(const packed_object_state_info_t &, size_t i, bool val) noexcept;
      void set_marked(const packed_object_state_info_t &, size_t i) noexcept;

      void *allocate(const packed_object_state_info_t &) noexcept;
      bool deallocate(const packed_object_state_info_t &, void *v) noexcept;

      void free_unmarked(const packed_object_state_info_t &) noexcept;

      auto num_blocks(const packed_object_state_info_t &) const noexcept -> size_t;
      auto free_bits(const packed_object_state_info_t &) noexcept -> bits_array_type *;
      auto free_bits(const packed_object_state_info_t &) const noexcept -> const bits_array_type *;
      auto mark_bits(const packed_object_state_info_t &) noexcept -> bits_array_type *;
      auto mark_bits(const packed_object_state_info_t &) const noexcept -> const bits_array_type *;

      //      ::std::array<integer_block_t<8>, num_blocks_needed()> m_free_bits;
      //      ::std::array<integer_block_t<8>, num_blocks_needed()> m_mark_bits;
    };
    static_assert(::std::is_pod<packed_object_state_t>::value, "");
  }
}
#include "packed_object_state_impl.hpp"
