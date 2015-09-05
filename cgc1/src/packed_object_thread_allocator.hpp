#pragma once
#include <cgc1/declarations.hpp>
#include "packed_object_state.hpp"
#include "internal_allocator.hpp"
namespace cgc1
{
  namespace details
  {
    static constexpr const size_t c_packed_object_block_size = 4096 * 128;
    class packed_object_allocator_t;

    static inline size_t get_packed_object_size_id(size_t sz);

    class packed_object_package_t
    {
    public:
      static constexpr const size_t cs_total_size = c_packed_object_block_size;

      using vector_type = cgc_internal_vector_t<packed_object_state_t *>;
      using array_type = ::std::array<vector_type, 5>;

      void insert(packed_object_package_t &&state);
      void insert(size_t id, packed_object_state_t *state);

      auto allocate(size_t id) noexcept -> void *;

      constexpr static packed_object_state_info_t _get_info(size_t id);

    private:
      array_type m_vectors;
    };

    class packed_object_thread_allocator_t
    {
    public:
      packed_object_thread_allocator_t(packed_object_allocator_t *allocator);
      packed_object_thread_allocator_t(const packed_object_thread_allocator_t &) = delete;
      packed_object_thread_allocator_t(packed_object_thread_allocator_t &&) noexcept = default;
      packed_object_thread_allocator_t &operator=(const packed_object_thread_allocator_t &) = delete;
      packed_object_thread_allocator_t &operator=(packed_object_thread_allocator_t &&) = default;

      auto allocate(size_t sz) noexcept -> void *;
      auto deallocate(void *v) noexcept -> bool;

    private:
      packed_object_package_t m_locals;
      cgc_internal_vector_t<void *> m_free_list;
      packed_object_allocator_t *m_allocator = nullptr;
    };
  }
}
#include "packed_object_thread_allocator_impl.hpp"
