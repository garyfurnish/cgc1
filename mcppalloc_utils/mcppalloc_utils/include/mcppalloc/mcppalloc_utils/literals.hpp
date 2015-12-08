#pragma once
namespace mcppalloc
{
  namespace literals
  {
    constexpr inline ::std::size_t operator"" _sz(unsigned long long x)
    {
      return static_cast<::std::size_t>(x);
    }
  }
}
