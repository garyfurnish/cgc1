#pragma once
#include <array>
#include <atomic>
#include <type_traits>
namespace mcppalloc
{
  namespace bitmap
  {
    namespace details
    {
      template <size_t Quads>
      struct alignas(32) integer_block_t {
        /**
         * \brief Number of quad words in integer block.
         **/
        static constexpr const size_t cs_quad_words = Quads;
        using value_type = uint64_t;

        static_assert(cs_quad_words % 8 == 0, "Number of quad words must be divisible by 8.");

        void clear() noexcept;
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
        void get_bit_atomic(size_t i, ::std::memory_order ordering) const noexcept;
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

        integer_block_t operator~() const noexcept;
        integer_block_t &negate() noexcept;

        integer_block_t operator|(const integer_block_t &) const noexcept;
        integer_block_t &operator|=(const integer_block_t &) noexcept;

        integer_block_t operator&(const integer_block_t &)const noexcept;
        integer_block_t &operator&=(const integer_block_t &) noexcept;

        static constexpr size_t size() noexcept;
        static constexpr size_t size_in_bytes() noexcept;
        static constexpr size_t size_in_bits() noexcept;

        ::std::array<value_type, cs_quad_words> m_array;
      };
      static_assert(::std::is_pod<integer_block_t<8>>::value, "");
    }
  }
}
#include "integer_block_impl.hpp"
