#pragma once
namespace std
{
  template <typename F, typename Tuple, size_t... I>
  MCPPALLOC_ALWAYS_INLINE auto apply_impl(F &&f, Tuple &&t, integer_sequence<size_t, I...>)
      -> decltype(::std::forward<F>(f)(::std::get<I>(::std::forward<Tuple>(t))...))
  {
    return t, ::std::forward<F>(f)(::std::get<I>(::std::forward<Tuple>(t))...);
  }
  template <typename F, typename Tuple>
  MCPPALLOC_ALWAYS_INLINE auto apply(F &&f, Tuple &&t)
  {
    return apply_impl(::std::forward<F>(f), ::std::forward<Tuple>(t),
                      make_index_sequence<::std::tuple_size<typename ::std::decay<Tuple>::type>::value>());
  }
}
