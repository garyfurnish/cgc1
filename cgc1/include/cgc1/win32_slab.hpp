#pragma once
#include "declarations.hpp"
#ifdef _WIN32
namespace cgc1
{
  /**
   * \brief Slab allocator backed around virtual alloc.
   **/
  class slab_t
  {
  public:
    using value_type = uint8_t;
    using reference = uint8_t &;
    using const_reference = const uint8_t &;
    using pointer = uint8_t *;
    using const_pointer = const uint8_t *;
    using iterator = uint8_t *;
    using const_iterator = const uint8_t *;
    using difference_type = ::std::ptrdiff_t;
    using size_type = size_t;

    slab_t() = default;
    slab_t(slab_t &) = delete;
    slab_t(slab_t &&) = default;
    slab_t &operator=(const slab_t &) = delete;
    slab_t &operator=(slab_t &&) = default;
    /**
     * \brief Constructor.
     *
     * Constructor aborts on failure.
     * @param size Size of memory to allocate.
     * @param addr Address to allocate at.
    **/
    slab_t(size_t size, void *addr);
    /**
     * \brief Constructor aborts on failure.
     *
     * Constructor aborts on failure.
     * @param size Size of memory to allocate.
    **/
    slab_t(size_t size);
    ~slab_t();
    /**
     * \brief Return start address of memory slab.
    **/
    void *addr() const;
    /**
     * \brief Return the size of the slab.
    **/
    size_type size() const;
    /**
     * \brief Return true if the slab is valid, false otherwise.
    **/
    bool valid() const;
    /**
     * Perform slab allocation.
     *
     * @param size Size of slab.
     * @param addr Optional address to try to allocate at.
     * @return True on success, false on failure.
     **/
    bool allocate(size_t size, void *addr = nullptr);
    /**
     * \brief Attempt to expand the slab.
     * @return True on success, false on failure.
    **/
    bool expand(size_t size);
    /**
     * \brief Destroy the memory slab.
    **/
    void destroy();
    /**
     * \brief Return the memory page size on the system.
    **/
    static size_t page_size();
    /**
     * \brief Find a hole in the current memory layout of at least size.
     * This works by reserving the memory.
    **/
    static void *find_hole(size_t size);
    /**
     * \brief Return start address of memory slab.
    **/
    uint8_t *begin() const;
    /**
     * \brief Return end address of memory slab.
    **/
    uint8_t *end() const;

  private:
    /**
     * \brief Start address of slab.
     * May be nullptr.
     **/
    void *m_addr = nullptr;
    /**
     * \brief Actual size of slab.
     **/
    size_type m_size = 0;
    /**
     * \brief True if slab valid, false otherwise.
     **/
    bool m_valid = false;
    /**
     * \brief Page size on system.
     **/
    static size_t s_page_size;
  };
}
#include "win32_slab_impl.hpp"
#endif
