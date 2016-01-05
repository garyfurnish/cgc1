#pragma once
#include "declarations.hpp"
#include <mcppalloc/mcppalloc_bitmap/dynamic_bitmap_ref.hpp>
#include <mcppalloc/mcppalloc_bitmap/integer_block.hpp>
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {

      using type_id_t = uint32_t;
      static constexpr const size_t c_bitmap_alignment = 32;
      struct alignas(size_t) bitmap_state_info_t {
        size_t m_num_blocks; //8 
        size_t m_data_entry_sz; //16
        size_t m_size; //24
        size_t m_header_size; //32
	size_t m_cached_first_free; //40
        type_id_t m_type_id; //44
	uint8_t m_num_user_bit_fields; 
	uint8_t m_pad2[3];
      };

      struct alignas(c_bitmap_alignment) bitmap_state_internal_t {
        size_t m_pre_magic_number;
        bitmap_state_info_t m_info;
        size_t m_post_magic_number;
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

        using bits_array_type = bitmap::details::integer_block_t<8>;

        auto declared_entry_size() const noexcept -> size_t;
        auto real_entry_size() const noexcept -> size_t;
        auto header_size() const noexcept -> size_t;
        /**
         * \brief Initialize part of state that all blocks share.
         **/
        void initialize_consts() noexcept;
        /**
         * \brief Initialize part of state that is argument specific.
         * @param type_id Type of state.
         **/
        void initialize(type_id_t type_id, uint8_t user_bit_fields) noexcept;
        void clear_mark_bits() noexcept;
        void clear_user_bits(size_t index) noexcept;

        auto num_bit_arrays() const noexcept -> size_t;

        auto any_free() const noexcept -> bool;
        auto none_free() const noexcept -> bool;
        auto all_free() const noexcept -> bool;
	/**
	 * \brief Return cached value that is first free index.
	 **/
        auto first_free() const noexcept -> size_t;
	/**
	 * \brief Recompute cached first free index.
	 **/
	auto _compute_first_free() noexcept -> size_t;
        auto any_marked() const noexcept -> bool;
        auto none_marked() const noexcept -> bool;
        auto free_popcount() const noexcept -> size_t;

        auto is_free(size_t i) const noexcept -> bool;
        auto is_marked(size_t i) const noexcept -> bool;

        auto type_id() const noexcept -> type_id_t;
        /**
         * \brief Return size in number of possible allocations.
         **/
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

        void or_with_to_be_freed(bitmap::dynamic_bitmap_ref_t<false> ref);
        void free_unmarked();
        /**
         * \brief Return number of blocks in each bit field.
         **/
        auto num_blocks() const noexcept -> size_t;
        /**
         * \brief Return number of user bit fields[
         **/
        auto num_user_bit_fields() const noexcept -> size_t;
        /**
         * \brief Return the size of blocks in bytes.
         **/
        auto block_size_in_bytes() const noexcept -> size_t;
        auto free_bits() noexcept -> bits_array_type *;
        auto free_bits() const noexcept -> const bits_array_type *;
        auto mark_bits() noexcept -> bits_array_type *;
        auto mark_bits() const noexcept -> const bits_array_type *;

        auto free_bits_ref() noexcept -> bitmap::dynamic_bitmap_ref_t<false>;
        auto free_bits_ref() const noexcept -> bitmap::dynamic_bitmap_ref_t<true>;
        auto mark_bits_ref() noexcept -> bitmap::dynamic_bitmap_ref_t<false>;
        auto mark_bits_ref() const noexcept -> bitmap::dynamic_bitmap_ref_t<true>;
        auto user_bits_ref(size_t index) noexcept -> bitmap::dynamic_bitmap_ref_t<false>;
        auto user_bits_ref(size_t index) const noexcept -> bitmap::dynamic_bitmap_ref_t<true>;

        /**
         * \brief Return the ith array of user bits.
         **/
        auto user_bits(size_t i) noexcept -> bits_array_type *;
        /**
         * \brief Return the ith array of user bits.
         **/
        auto user_bits(size_t i) const noexcept -> const bits_array_type *;
        /**
         * \brief Return the ith array of user bits with bound check.
         **/
        auto user_bits_checked(size_t i) noexcept -> bits_array_type *;
        /**
         * \brief Return the ith array of user bits with bound check.
         **/
        auto user_bits_checked(size_t i) const noexcept -> const bits_array_type *;

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
