#pragma once
namespace mcppalloc
{
  namespace bitmap
  {
    class integer_block_t;
    template <bool is_const>
    class dynamic_bitmap_ref_t
    {
    public:
      using bits_array_type = bitmap::details::integer_block_t<8>;
      template <bool E = is_const>
      dynamic_bitmap_ref_t(typename ::std::enable_if<E, const bits_array_type *>::type array, size_t sz) noexcept;
      dynamic_bitmap_ref_t(bits_array_type *array, size_t sz) noexcept;
      dynamic_bitmap_ref_t(const dynamic_bitmap_ref_t &) noexcept;
      dynamic_bitmap_ref_t(dynamic_bitmap_ref_t &&) noexcept;
      dynamic_bitmap_ref_t &operator=(const dynamic_bitmap_ref_t &) noexcept;
      dynamic_bitmap_ref_t &operator=(dynamic_bitmap_ref_t &&) noexcept;

      auto size() const noexcept -> size_t;
      auto size_in_bytes() const noexcept -> size_t;
      auto size_in_bits() const noexcept -> size_t;
      auto array() noexcept -> bits_array_type *;
      auto array() const noexcept -> const bits_array_type *;

      template <bool E = !is_const>
      typename ::std::enable_if<E, void>::type clear() noexcept;
      void fill(uint64_t word) noexcept;
      /**
       * \brief Set the ith bit to value.
       **/
      void set_bit(size_t i, bool value) noexcept;
      /**
       * \brief Set the ith bit to the value atomically.
       **/
      void set_bit_atomic(size_t i, bool value, ::std::memory_order ordering) noexcept;
      /**
       * \brief Get the ith bit.
       **/
      auto get_bit(size_t i) const noexcept -> bool;
      /**
       * \brief Set the ith bit to the value atomically.
       **/
      auto get_bit_atomic(size_t i, ::std::memory_order ordering) const noexcept -> bool;
      /**
       * \brief Return true if any bit is set.
       **/
      auto any_set() const noexcept -> bool;
      /**
       * \brief Return true if all bits are set.
       **/
      auto all_set() const noexcept -> bool;
      /**
       * \brief Return true if no bit is set.
       **/
      auto none_set() const noexcept -> bool;
      /**
       * \brief Return the first bit that is set.
       **/
      auto first_set() const noexcept -> size_t;
      /**
       * \brief Return the first bit that is not set.
       **/
      auto first_not_set() const noexcept -> size_t;
      /**
       * \brief Return number of set bits.
       **/
      auto popcount() const noexcept -> size_t;

      dynamic_bitmap_ref_t &negate() noexcept;

      dynamic_bitmap_ref_t &operator|=(const dynamic_bitmap_ref_t &) noexcept;

      dynamic_bitmap_ref_t &operator&=(const dynamic_bitmap_ref_t &) noexcept;

      dynamic_bitmap_ref_t &operator^=(const dynamic_bitmap_ref_t &) noexcept;

    private:
      typename ::std::conditional<is_const, const bits_array_type *, bits_array_type *>::type m_array;
      size_t m_size;
    };
  }
}
#include "dynamic_bitmap_impl.hpp"
