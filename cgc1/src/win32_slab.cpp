#ifdef _WIN32
#include <cgc1/win32_slab.hpp>
#define NOMINMAX
#include <Windows.h>
#include <assert.h>
namespace cgc1
{
  bool slab_t::allocate(size_t size, void *addr)
  {
    if (size == 0)
      return false;
    size = align(size, page_size());
    _MEMORY_BASIC_INFORMATION information{};
    void *ret = nullptr;
    if (addr) {
      if (!::VirtualQuery(ret, &information, sizeof(MEMORY_BASIC_INFORMATION)))
        ::abort();
      if (information.State | MEM_RESERVE)
        ret = ::VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
      else
        ret = ::VirtualAlloc(addr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }
    else
      ret = ::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!ret) {
      return false;
    }
    if (!::VirtualQuery(ret, &information, sizeof(MEMORY_BASIC_INFORMATION))) {
      ::abort();
    }
    assert(information.State | MEM_COMMIT);
    // should never happen.
    if (size < information.RegionSize)
      ::abort();
    m_size = information.RegionSize;
    m_addr = ret;
    m_valid = true;
    return ret != nullptr;
  }
  bool slab_t::expand(size_t size)
  {
    if (size <= m_size)
      return true;
    if (!m_addr)
      return false;
    size = align(size, page_size());
    _MEMORY_BASIC_INFORMATION information{};
    if (!::VirtualQuery(end(), &information, sizeof(MEMORY_BASIC_INFORMATION))) {
      ::abort();
    }
    void *ret = nullptr;
    if (information.State & MEM_COMMIT) {
      assert(0);
      return false;
    }
    if (information.State & MEM_RESERVE) {
      ret = ::VirtualAlloc(end(), size, MEM_COMMIT, PAGE_READWRITE);
    }
    else {
      ret = ::VirtualAlloc(end(), size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }
    if (ret != end()) {
      assert(0);
      return false;
    }
    m_size = size;
    return true;
  }
  void slab_t::destroy()
  {
    if (m_addr) {
      int ret = ::VirtualFree(m_addr, m_size, MEM_RELEASE);
      if (ret)
        assert(0);
    }
    m_valid = false;
    m_addr = nullptr;
    m_size = 0;
  }
  size_t slab_t::page_size()
  {
    size_t size = 4096; //::GetLargePageMinimum();
    if (!size)
      ::abort();
    return size;
  }
  void *slab_t::find_hole(size_t size)
  {
    size = align(size, 2 * page_size());
    void *addr = ::VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);
    return addr;
  }
}
#endif