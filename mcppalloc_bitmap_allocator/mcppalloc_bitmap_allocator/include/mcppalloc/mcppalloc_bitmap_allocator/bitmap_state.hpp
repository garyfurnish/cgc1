#pragma once
#include "declarations.hpp"
#include <mcppalloc/mcppalloc_bitmap/integer_block.hpp>
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {

      using type_id_t = size_t;
      static constexpr const size_t c_bitmap_alignment = 32;
      struct alignas(size_t) bitmap_state_info_t {
        size_t m_num_blocks;
        size_t m_data_entry_sz;
        size_t m_size;
        size_t m_header_size;
        type_id_t m_type_id;
      };

      struct alignas(c_bitmap_alignment) bitmap_state_internal_t {
        size_t m_pre_magic_number;
        bitmap_state_info_t m_info;
        size_t m_post_magic_number[2];
      };

      static_assert(sizeof(bitmap_state_internal_t) == 2 * c_bitmap_alignment, "");

      /**
       * \brief Universal block size.
       **/
      static constexpr const size_t c_bitmap_block_size = 4096 * 128;

      struct alignas(c_bitmap_alignment) bitmap_state_t {
      public:
        static const constexpr size_t cs_header_alignment = 32;
        static const constexpr size_t cs_object_alignment = 32;
        static const constexpr size_t cs_bits_array_multiple = 2;
        static const constexpr size_t cs_magic_number_pre = 0xd7c3f4bb0bea958c;
        static const constexpr size_t cs_magic_number_0 = 0xa58d0aebb1fae1d9;
        static const constexpr size_t cs_magic_number_1 = 0x164df5314ffcf804;

        using bits_array_type = bitmap::details::integer_block_t<8>;

        auto declared_entry_size() const noexcept -> size_t;
        auto real_entry_size() const noexcept -> size_t;
        auto header_size() const noexcept -> size_t;

        void initialize_consts() noexcept;
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

        auto type_id() const noexcept -> type_id_t;

        auto size() const noexcept -> size_t;
        void _compute_size() noexcept;
        auto size_bytes() const noexcept -> size_t;
        auto total_size_bytes() const noexcept -> size_t;
        auto begin() noexcept -> uint8_t *;
        auto end() noexcept -> uint8_t *;
        auto begin() const noexcept -> const uint8_t *;
        auto end() const noexcept -> const uint8_t *;

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

        auto has_valid_magic_numbers() const noexcept -> bool;
        void verify_magic() const;

        auto get_index(void *v) const noexcept -> size_t;
        auto addr_in_header(void *v) const noexcept -> bool;
        auto get_object(size_t i) noexcept -> void *;

        bitmap_state_internal_t m_internal;
      };
      static_assert(::std::is_pod<bitmap_state_t>::value, "");
      static_assert(sizeof(bitmap_state_internal_t) == c_bitmap_alignment * 2, "");

      extern bitmap_state_t *get_state(void *v);
    }
  }
}
#include "bitmap_state_impl.hpp"
