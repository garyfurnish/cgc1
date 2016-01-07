#pragma once
#include "cgc_root.hpp"
namespace cgc1
{
  template <typename T>
  class CGC1_DLL_PUBLIC cgc_root_pointer_t
  {
  public:
    using pointer_type = T *;
    using const_pointer_type = const T *;
    using reference_type = T &;
    using const_reference_type = const T &;
    cgc_root_pointer_t() : m_root(m_t)
    {
    }
    cgc_root_pointer_t(T *t) : m_t(t), m_root(m_t)
    {
    }
    cgc_root_pointer_t(const cgc_root_pointer_t<T> &) noexcept = delete;
    cgc_root_pointer_t(cgc_root_pointer_t<T> &&) noexcept = default;
    cgc_root_pointer_t &operator=(const cgc_root_pointer_t<T> &) noexcept = delete;
    cgc_root_pointer_t &operator=(cgc_root_pointer_t<T> &&) noexcept = default;
    auto operator*() noexcept -> reference_type
    {
      return *m_t;
    }
    auto operator*() const noexcept -> const_reference_type
    {
      return *m_t;
    }
    auto operator-> () noexcept -> pointer_type
    {
      return m_t;
    }
    auto operator-> () const noexcept -> const_pointer_type
    {
      return m_t;
    }
    auto operator&() noexcept -> pointer_type *
    {
      return &m_t;
    }
    auto operator&() const noexcept -> const_pointer_type *
    {
      return &m_t;
    }
    auto ptr() noexcept -> pointer_type
    {
      return m_t;
    }
    operator pointer_type() noexcept
    {
      return m_t;
    }
    operator const_pointer_type() const noexcept
    {
      return m_t;
    }
    void clear_root()
    {
      m_root.clear();
    }

  private:
    pointer_type m_t{nullptr};
    cgc_root_t m_root;
  };
}
