#pragma once
#include <cgc1/declarations.hpp>
#include "packed_object_state.hpp"
namespace cgc1
{
  namespace details
  {
    static inline size_t get_packed_object_size_id(size_t sz);

    class packed_object_package_t
    {
    public:
      static constexpr const size_t cs_total_size = 4096 * 8;

      /*      ::std::vector<::std::pair<void *, packed_object_state_t<cs_total_size, 32, 512> *>> m_32;
      ::std::vector<::std::pair<void *, packed_object_state_t<cs_total_size, 64, 256> *>> m_64;
      ::std::vector<::std::pair<void *, packed_object_state_t<cs_total_size, 128, 128> *>> m_128;
      ::std::vector<::std::pair<void *, packed_object_state_t<cs_total_size, 256, 128> *>> m_256;
      ::std::vector<::std::pair<void *, packed_object_state_t<cs_total_size, 512, 128> *>> m_512;
      */
      void insert(const packed_object_package_t &state);
    };

    class packed_object_thread_allocator_t
    {
    public:
      packed_object_thread_allocator_t() = default;
      packed_object_thread_allocator_t(const packed_object_thread_allocator_t &) = delete;
      packed_object_thread_allocator_t(packed_object_thread_allocator_t &&) noexcept = default;
      packed_object_thread_allocator_t &operator=(const packed_object_thread_allocator_t &) = delete;
      packed_object_thread_allocator_t &operator=(packed_object_thread_allocator_t &&) = default;

      void allocate(size_t sz);
      void deallocate(void *v);

    private:
      packed_object_package_t m_locals;
      packed_object_package_t m_free_lists;
    };
  }
}
#include "packed_object_thread_allocator_impl.hpp"
