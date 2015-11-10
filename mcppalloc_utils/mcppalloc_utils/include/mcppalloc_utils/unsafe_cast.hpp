#pragma once
namespace mcppalloc
{
  /**
   * \brief Unsafe cast of references.
   *
   * Contain any undefined behavior here here.
   * @tparam Type New type.
   **/
  template <typename Type, typename In>
  Type &unsafe_reference_cast(In &in)
  {
    union {
      Type *t;
      In *i;
    } u;
    u.i = &in;
    return *u.t;
  }
  /**
   * \brief Unsafe cast of pointers
   *
   * Contain any undefined behavior here here.
   * @tparam Type New type.
   **/
  template <typename Type, typename In>
  Type *unsafe_cast(In *in)
  {
    union {
      Type *t;
      In *i;
    } u;
    u.i = in;
    return u.t;
  }
}
