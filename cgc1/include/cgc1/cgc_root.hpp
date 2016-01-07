#pragma once
#include <mcppalloc/mcppalloc_utils/unsafe_cast.hpp>
namespace cgc1
{
  class CGC1_DLL_PUBLIC cgc_root
  {
  public:
    template <typename T>
    cgc_root(T *&ptr) : m_root(mcppalloc::unsafe_cast<void *>(&ptr))
    {
      cgc_add_root(m_root);
    }
    ~cgc_root()
    {
      if (m_root)
        cgc_remove_root(m_root);
    }
    cgc_root(const cgc_root &) = delete;
    cgc_root(cgc_root &&cgc_root) noexcept : m_root(::std::move(cgc_root.m_root))
    {
    }
    cgc_root &operator=(const cgc_root &) = delete;
    cgc_root &operator=(cgc_root &&rhs) noexcept
    {
      m_root = rhs.m_root;
      rhs.m_root = m_root;
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
