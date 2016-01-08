#pragma once
#include "cgc_root.hpp"
namespace cgc1
{
  template <typename T>
  class CGC1_DLL_PUBLIC cgc_root_pointer_t
  {
  public:
    using pointer_type = T *;
    using reference_type = T &;
    cgc_root_pointer_t() : m_root(m_t)
    {
    }
    cgc_root_pointer_t(T *t) : m_t(t), m_root(m_t)
    {
    }
    cgc_root_pointer_t(const cgc_root_pointer_t &p) noexcept = delete;
    cgc_root_pointer_t(cgc_root_pointer_t &&) noexcept = default;
    cgc_root_pointer_t &operator=(pointer_type ptr) noexcept
    {
      m_t = ptr;
      return *this;
    }
    cgc_root_pointer_t &operator=(const cgc_root_pointer_t &rhs) noexcept
    {
      m_t = rhs.m_t;
      return *this;
    }
    cgc_root_pointer_t &operator=(cgc_root_pointer_t &&) noexcept = default;
    auto operator*() const noexcept -> reference_type
    {
      return *m_t;
    }
    auto operator-> () const noexcept -> pointer_type
    {
      return m_t;
    }
    auto operator&() noexcept -> pointer_type *
    {
      return &m_t;
    }
    auto operator&() const noexcept -> const pointer_type *
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
    void clear_root()
    {
      m_root.clear();
    }

  private:
    pointer_type m_t;
    cgc_root_t m_root;
  };
  template <typename T>
  class CGC1_DLL_PUBLIC cgc_root_pointer_converting_t
  {
  public:
    using pointer_type = T *;
    using reference_type = T &;
    cgc_root_pointer_converting_t() : m_root(m_t)
    {
    }
    cgc_root_pointer_converting_t(T *t) : m_t(t), m_root(m_t)
    {
    }
    cgc_root_pointer_converting_t(const cgc_root_pointer_converting_t &p) noexcept = delete;
    cgc_root_pointer_converting_t(cgc_root_pointer_converting_t &&) noexcept = default;
    cgc_root_pointer_converting_t &operator=(pointer_type ptr) noexcept
    {
      m_t = ptr;
      return *this;
    }
    cgc_root_pointer_converting_t &operator=(const cgc_root_pointer_converting_t &rhs) noexcept
    {
      m_t = rhs.m_t;
      return *this;
    }
    cgc_root_pointer_converting_t &operator=(cgc_root_pointer_converting_t &&) noexcept = default;
    auto operator*() const noexcept -> reference_type
    {
      return *m_t;
    }
    auto operator-> () const noexcept -> pointer_type
    {
      return m_t;
    }
    auto operator&() noexcept -> pointer_type *
    {
      return &m_t;
    }
    auto operator&() const noexcept -> const pointer_type *
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
    template <typename CONVERSION>
    explicit operator CONVERSION() noexcept
    {
      return reinterpret_cast<CONVERSION>(m_t);
    }
    void clear_root()
    {
      m_root.clear();
    }

  private:
    pointer_type m_t;
    cgc_root_t m_root;
  };

  template <typename T>
  using cgc_root_pointer2_t = cgc_root_pointer_t<typename ::std::remove_pointer<T>::type>;
  template <typename T>
  using cgc_root_pointer2_converting_t = cgc_root_pointer_converting_t<typename ::std::remove_pointer<T>::type>;
}
