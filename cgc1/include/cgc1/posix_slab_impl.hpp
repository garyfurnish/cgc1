#pragma once
#include "posix_slab.hpp"
#ifdef CGC1_POSIX
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include <iostream>
namespace cgc1
{
#ifdef __APPLE__
#ifndef SIGUNUSED
#define SIGUNUSED 31
#endif
  static const int c_map_anonymous = MAP_ANON;
#else
  static const int c_map_anonymous = MAP_ANONYMOUS;
#endif

  inline slab_t::slab_t(size_t size, void *addr)
  {
    if (!allocate(size, addr))
      abort();
  }
  inline slab_t::slab_t(size_t size)
  {
    if (!allocate(size))
      abort();
  }
  inline slab_t::~slab_t()
  {
    destroy();
  }
  inline void *slab_t::addr() const noexcept
  {
    return m_addr;
  }
  inline auto slab_t::size() const noexcept -> size_type
  {
    return m_size;
  }
  inline bool slab_t::valid() const noexcept
  {
    return m_valid;
  }
  inline bool slab_t::allocate(size_t size, void *addr)
  {
    // sanity check size.
    if (size == 0)
      return false;
    void *ret = nullptr;
    // attempt to map the memory.
    if (addr)
      ret = ::mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | c_map_anonymous | MAP_FIXED, -1, 0);
    else
      ret = ::mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | c_map_anonymous, -1, 0);
    // check of failure.
    if (!ret || ret == reinterpret_cast<void *>(-1)) {
      ::std::cerr << "\n Failed to allocate memory " << addr << " " << size << ::std::endl;
      return false;
    }
    m_addr = ret;
    m_size = size;
    m_valid = true;
    return ret != nullptr;
  }
#ifdef __APPLE__
  inline bool slab_t::expand(size_t size)
  {
    ::std::cerr << "Apple can not expand memory\n";
    ::std::cerr << "Requested size is " << size << "\n";
    ::std::cerr << "Apple lacks mremap\n";
    abort();
    return false;
  }
#else
  inline bool slab_t::expand(size_t size)
  {
    // sanity check size.
    if (size == 0)
      return true;
    // make sure memory has already been allocated.
    if (!m_addr)
      return false;
    // try to expand the memory.
    // this can fail for lots of reasons, such as ASLR.
    void *ret = ::mremap(m_addr, m_size, size, 0);
    if (ret != m_addr) {
      ::std::cerr << "\n Failed to expand memory " << m_addr << " " << m_size << " " << size << " returned " << ret
                  << ::std::endl;
      assert(0);
      return false;
    }
    m_size = size;
    return true;
  }
#endif
  inline void slab_t::destroy()
  {
    if (m_addr) {
      // if memory exists, unmap it.
      int ret = ::munmap(m_addr, m_size);
      if (ret)
        assert(0);
    }
    m_valid = false;
    m_addr = nullptr;
    m_size = 0;
  }
  inline size_t slab_t::page_size() noexcept
  {
    if (s_page_size)
      return s_page_size;
    // This is guarenteed to be positive by api.
    s_page_size = static_cast<size_t>(::getpagesize());
    return s_page_size;
  }
  inline void *slab_t::find_hole(size_t size)
  {
    // try to allocate some memory of requested size.
    void *addr = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_NORESERVE | MAP_PRIVATE | c_map_anonymous, -1, 0);
    if (addr) {
      // if it succeeded, immediately unmap it.
      int ret = ::munmap(addr, size);
      if (ret)
        assert(0);
      // address is known to be a good hole for a slab as long as nothing else is mmaped.
      return addr;
    }
    return nullptr;
  }
  inline uint8_t *slab_t::begin() const noexcept
  {
    return reinterpret_cast<uint8_t *>(m_addr);
  }
  inline uint8_t *slab_t::end() const noexcept
  {
    return reinterpret_cast<uint8_t *>(m_addr) + m_size;
  }
}
#endif
