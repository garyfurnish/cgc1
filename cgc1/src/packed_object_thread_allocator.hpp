#pragma once
#include <cgc1/declarations.hpp>
#include "packed_object_state.hpp"
#include "internal_allocator.hpp"
#include "packed_object_package.hpp"
namespace cgc1
{
  namespace details
  {

    class packed_object_thread_allocator_t
    {
    public:
      packed_object_thread_allocator_t(packed_object_allocator_t &allocator);
      packed_object_thread_allocator_t(const packed_object_thread_allocator_t &) = delete;
      packed_object_thread_allocator_t(packed_object_thread_allocator_t &&) noexcept = default;
      packed_object_thread_allocator_t &operator=(const packed_object_thread_allocator_t &) = delete;
      packed_object_thread_allocator_t &operator=(packed_object_thread_allocator_t &&) = default;
      ~packed_object_thread_allocator_t();

      auto allocate(size_t sz) noexcept -> void *;
      auto deallocate(void *v) noexcept -> bool;
      auto destroy(void *v) noexcept -> bool
      {
        return deallocate(v);
      }

      void set_max_in_use(size_t max_in_use);
      void set_max_free(size_t max_free);
      auto max_in_use() const noexcept -> size_t;
      auto max_free() const noexcept -> size_t;
      void do_maintenance();

      template <typename Predicate>
      void for_all_state(Predicate &&predicate);

    private:
      packed_object_package_t m_locals;
      cgc_internal_vector_t<void *> m_free_list;
      packed_object_allocator_t &m_allocator;
      ::std::array<size_t, packed_object_package_t::cs_num_vectors> m_popcount_max;
      size_t m_max_in_use = 10;
      size_t m_max_free = 5;
    };
  }
}
#ifdef CGC1_INLINES
#include "packed_object_thread_allocator_impl.hpp"
#endif
#include "packed_object_thread_allocator_timpl.hpp"
