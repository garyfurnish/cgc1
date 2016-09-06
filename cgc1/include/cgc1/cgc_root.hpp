#pragma once
#include <mcpputil/mcpputil/unsafe_cast.hpp>
namespace cgc1
{
  class CGC1_DLL_PUBLIC cgc_root_t
  {
  public:
    template <typename T>
    cgc_root_t(T *&ptr) : m_root(mcpputil::unsafe_cast<void *>(&ptr))
    {
      cgc_add_root(m_root);
    }
    ~cgc_root_t()
    {
      if (m_root)
        cgc_remove_root(m_root);
    }
    cgc_root_t(const cgc_root_t &) = delete;
    cgc_root_t(cgc_root_t &&cgc_root) noexcept : m_root(::std::move(cgc_root.m_root))
    {
      cgc_root.m_root = nullptr;
    }
    cgc_root_t &operator=(const cgc_root_t &) = delete;
    cgc_root_t &operator=(cgc_root_t &&rhs) noexcept
    {
      m_root = rhs.m_root;
      rhs.m_root = nullptr;
      return *this;
    }
    auto root() const noexcept -> void *
    {
      return m_root;
    }
    void clear()
    {
      cgc_remove_root(m_root);
      m_root = nullptr;
    }

  private:
    void **m_root;
  };
}
