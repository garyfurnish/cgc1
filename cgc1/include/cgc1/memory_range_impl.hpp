#pragma once
namespace cgc1
{
  template <typename Pointer_Type>
  memory_range_t<Pointer_Type>::memory_range_t() noexcept(::std::is_nothrow_default_constructible<pointer_type>::value) = default;
  template <typename Pointer_Type>
  memory_range_t<Pointer_Type>::memory_range_t(pointer_type begin, pointer_type end) noexcept(
      ::std::is_nothrow_move_constructible<pointer_type>::value)
      : m_begin(::std::move(begin)), m_end(::std::move(end))
  {
  }
  template <typename Pointer_Type>
  memory_range_t<Pointer_Type>::memory_range_t(::std::pair<pointer_type, pointer_type> pair) noexcept(
      ::std::is_nothrow_move_constructible<pointer_type>::value)
      : m_begin(::std::move(pair.first)), m_end(::std::move(pair.second))
  {
  }
  template <typename Pointer_Type>
  void
  memory_range_t<Pointer_Type>::set_begin(pointer_type begin) noexcept(::std::is_nothrow_move_assignable<pointer_type>::value)
  {
    m_begin = ::std::move(begin);
  }
  template <typename Pointer_Type>
  void memory_range_t<Pointer_Type>::set_end(pointer_type end) noexcept(::std::is_nothrow_move_assignable<pointer_type>::value)
  {
    m_end = ::std::move(end);
  }
  template <typename Pointer_Type>
  void memory_range_t<Pointer_Type>::set(pointer_type begin,
                                         pointer_type end) noexcept(::std::is_nothrow_move_assignable<pointer_type>::value)
  {
    m_begin = ::std::move(begin);
    m_end = ::std::move(end);
  }
  template <typename Pointer_Type>
  void memory_range_t<Pointer_Type>::set(::std::pair<pointer_type, pointer_type> pair) noexcept(
      ::std::is_nothrow_move_assignable<pointer_type>::value)
  {
    m_begin = ::std::move(pair.first);
    m_end = ::std::move(pair.second);
  }
  template <typename Pointer_Type>
  auto memory_range_t<Pointer_Type>::begin() const noexcept(::std::is_nothrow_copy_constructible<pointer_type>::value)
      -> pointer_type
  {
    return m_begin;
  }
  template <typename Pointer_Type>
  auto memory_range_t<Pointer_Type>::end() const noexcept(::std::is_nothrow_copy_constructible<pointer_type>::value)
      -> pointer_type
  {
    return m_end;
  }
  template <typename Pointer_Type>
  auto memory_range_t<Pointer_Type>::size() const noexcept -> size_type
  {
    return static_cast<size_type>(end() - begin());
  }
  template <typename Pointer_Type>
  auto memory_range_t<Pointer_Type>::empty() const noexcept -> bool
  {
    return size() == 0;
  }
  template <typename Pointer_Type>
  auto memory_range_t<Pointer_Type>::contains(const pointer_type &ptr) const noexcept -> bool
  {
    return ptr >= begin() && ptr < end();
  }
  template <typename Pointer_Type>
  auto memory_range_t<Pointer_Type>::contains(const memory_range_t<pointer_type> &range) const noexcept -> bool
  {
    return begin() <= range.begin() && range.end() <= end();
  }

  template <typename Pointer_Type>
  auto memory_range_t<Pointer_Type>::size_comparator() noexcept
  {
    return [](const memory_range_t<pointer_type> &a, const memory_range_t<pointer_type> &b) noexcept->bool
    {
      return a.size() < b.size();
    };
  }
  template <typename Pointer_Type>
  auto memory_range_t<Pointer_Type>::make_nullptr() noexcept -> memory_range_t<pointer_type>
  {
    return memory_range_t<pointer_type>(nullptr, nullptr);
  }
  template <typename Pointer_Type>
  auto operator==(const memory_range_t<Pointer_Type> &lhs, const memory_range_t<Pointer_Type> &rhs) noexcept -> bool
  {
    return lhs.begin() == rhs.begin() && lhs.end() == rhs.end();
  }
  template <typename Pointer_Type>
  auto operator!=(const memory_range_t<Pointer_Type> &lhs, const memory_range_t<Pointer_Type> &rhs) noexcept -> bool
  {
    return lhs.begin() != rhs.begin() || lhs.end != rhs.end();
  }
  template <typename Pointer_Type>
  auto operator<(const memory_range_t<Pointer_Type> &lhs, const memory_range_t<Pointer_Type> &rhs) noexcept -> bool
  {
    return lhs.begin() < rhs.begin();
  }
  template <typename Pointer_Type>
  ::std::ostream &operator<<(::std::ostream &stream, const memory_range_t<Pointer_Type> &range)
  {
    stream << "(" << reinterpret_cast<void *>(range.begin()) << "," << reinterpret_cast<void *>(range.end()) << ")";
    return stream;
  }
  namespace details
  {
    template <typename Pointer_Type>
    auto size(const cgc1::memory_range_t<Pointer_Type> &range)
    {
      return range.size();
    }
  }
}
