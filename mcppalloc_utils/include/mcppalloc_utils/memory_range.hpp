#pragma once
namespace mcppalloc
{

  template <typename Pointer_Type>
  class memory_range_t
  {
  public:
    using pointer_type = Pointer_Type;
    using difference_type = typename ::std::pointer_traits<pointer_type>::difference_type;
    using size_type = size_t;
    static_assert(::std::numeric_limits<size_type>::max() > ::std::numeric_limits<difference_type>::max(), "");

    memory_range_t() noexcept(::std::is_nothrow_default_constructible<pointer_type>::value);
    memory_range_t(pointer_type begin, pointer_type end) noexcept(::std::is_nothrow_move_constructible<pointer_type>::value);
    memory_range_t(::std::pair<pointer_type, pointer_type> pair) noexcept(
        ::std::is_nothrow_move_constructible<pointer_type>::value);

    void set_begin(pointer_type begin) noexcept(::std::is_nothrow_move_assignable<pointer_type>::value);
    void set_end(pointer_type end) noexcept(::std::is_nothrow_move_assignable<pointer_type>::value);
    void set(pointer_type begin, pointer_type end) noexcept(::std::is_nothrow_move_assignable<pointer_type>::value);
    void set(::std::pair<pointer_type, pointer_type> pair) noexcept(::std::is_nothrow_move_assignable<pointer_type>::value);

    auto begin() const noexcept(::std::is_nothrow_copy_constructible<pointer_type>::value) -> pointer_type;
    auto end() const noexcept(::std::is_nothrow_copy_constructible<pointer_type>::value) -> pointer_type;
    auto size() const noexcept -> size_type;
    auto empty() const noexcept -> bool;

    auto contains(const pointer_type &ptr) const noexcept -> bool;
    auto contains(const memory_range_t<pointer_type> &ptr) const noexcept -> bool;

    static auto size_comparator() noexcept;
    static auto make_nullptr() noexcept -> memory_range_t<pointer_type>;

    operator ::std::pair<pointer_type, pointer_type>() const noexcept
    {
      return ::std::make_pair(m_begin, m_end);
    }

  private:
    pointer_type m_begin;
    pointer_type m_end;
  };
  template <typename Pointer_Type>
  auto operator==(const memory_range_t<Pointer_Type> &lhs, const memory_range_t<Pointer_Type> &rhs) noexcept -> bool;
  template <typename Pointer_Type>
  auto operator!=(const memory_range_t<Pointer_Type> &lhs, const memory_range_t<Pointer_Type> &rhs) noexcept -> bool;
  template <typename Pointer_Type>
  auto operator<(const memory_range_t<Pointer_Type> &lhs, const memory_range_t<Pointer_Type> &rhs) noexcept -> bool;

  using system_memory_range_t = memory_range_t<uint8_t *>;
}
#include "memory_range_impl.hpp"
