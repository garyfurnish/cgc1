#pragma once
#include "internal_allocator.hpp"
#include <mcppalloc_sparse/allocator.hpp>
#include <mcppalloc_utils/security.hpp>
#include "gc_user_data.hpp"
namespace cgc1
{
  namespace details
  {

    /**
     * \brief Allocator thread policy for garbage collector.
     *
     * In particular note that gc user data is used.
     **/
    struct gc_allocator_thread_policy_t : public ::mcppalloc::details::allocator_thread_policy_tag_t {
      void on_allocation(void *addr, size_t sz)
      {
        ::mcppalloc::secure_zero(addr, sz);
      }
      ::mcppalloc::do_nothing_t on_create_allocator_block;
      ::mcppalloc::do_nothing_t on_destroy_allocator_block;
      ::mcppalloc::do_nothing_t on_creation;
      auto on_allocation_failure(const ::mcppalloc::details::allocation_failure_t &failure)
          -> ::mcppalloc::details::allocation_failure_action_t;

      using allocator_block_user_data_type = gc_user_data_t;
    };
    struct gc_allocator_policy_t : public ::mcppalloc::allocator_policy_tag_t {
      using uintptr_type = uintptr_t;
      using pointer_type = void *;
      using size_type = size_t;
      using ptrdiff_type = ptrdiff_t;
      using internal_allocator_type = cgc_internal_allocator_t<void>;
      using user_data_type = details::gc_user_data_t;
      using thread_policy_type = gc_allocator_thread_policy_t;
      static const constexpr size_type cs_minimum_alignment = 16;
      gc_allocator_policy_t() = delete;
    };
    /**
     * \brief GC allocator type.
    **/
    using gc_allocator_t = ::mcppalloc::sparse::allocator_t<gc_allocator_policy_t>;
    /**
     * \brief Type of object state for gc sparse allocator.
     **/
    using gc_sparse_object_state_t = gc_allocator_t::object_state_type;
    /**
     * \brief Return true if the object state is marked, false otherwise.
    **/
    extern bool is_marked(const gc_sparse_object_state_t *os);
    /**
     * \brief Return true if the memory pointed to by the object state is atomic, false otherwise.
    **/
    extern bool is_atomic(const gc_sparse_object_state_t *os);
    /**
     * \brief Return true if the memory pointed to by the object state is complicated and needs special handeling.
    **/
    extern bool is_complex(const gc_sparse_object_state_t *os);
    /**
     * \brief Clear the gc mark for a given object state.
    **/
    extern void clear_mark(gc_sparse_object_state_t *os);
    /**
     * \brief Set the gc mark for a given object state.
    **/
    extern void set_mark(gc_sparse_object_state_t *os, bool status = true);
    /**
     * \brief Set the atomic flag for a given object state.
    **/
    extern void set_atomic(gc_sparse_object_state_t *os, bool status);
    /**
     * \brief Set the complex memory flag for a given object state.
    **/
    extern void set_complex(gc_sparse_object_state_t *os, bool status);
  }
}
#ifdef CGC1_INLINES
#include "gc_allocator_impl.hpp"
#endif
