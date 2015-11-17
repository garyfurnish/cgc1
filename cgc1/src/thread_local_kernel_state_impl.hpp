#pragma once
#include "gc_allocator.hpp"
#include "thread_local_kernel_state.hpp"
#include <mcppalloc/object_state.hpp>
namespace cgc1
{
  namespace details
  {
#ifdef __APPLE__
    extern pthread_key_t g_pkey;
    inline void initialize_tlks()
    {
      pthread_key_create(&g_pkey, [](void *) {});
    }
    inline void destroy_tlks()
    {
      pthread_key_delete(g_pkey);
    }
    inline thread_local_kernel_state_t *get_tlks()
    {
      return reinterpret_cast<thread_local_kernel_state_t *>(pthread_getspecific(g_pkey));
    }
    inline void set_tlks(thread_local_kernel_state_t *tlks)
    {
      pthread_setspecific(g_pkey, tlks);
    }
#else
#ifndef _WIN32
    extern thread_local thread_local_kernel_state_t *t_tlks;
#else
    extern __declspec(thread) thread_local_kernel_state_t *t_tlks;
#endif
    inline void initialize_tlks()
    {
    }
    inline void destroy_tlks()
    {
    }
    inline void set_tlks(thread_local_kernel_state_t *tlks)
    {
      t_tlks = tlks;
    }
    inline thread_local_kernel_state_t *get_tlks()
    {
      return t_tlks;
    }
#endif
    inline uint8_t *thread_local_kernel_state_t::top_of_stack() const
    {
      return m_top_of_stack;
    }
    inline void thread_local_kernel_state_t::set_top_of_stack(void *top)
    {
      m_top_of_stack = reinterpret_cast<uint8_t *>(top);
    }
    inline void thread_local_kernel_state_t::set_stack_ptr(void *stack_ptr)
    {
      m_stack_ptr = reinterpret_cast<uint8_t *>(stack_ptr);
    }
    inline uint8_t *thread_local_kernel_state_t::current_stack_ptr() const
    {
      return m_stack_ptr;
    }
    inline void thread_local_kernel_state_t::set_in_signal_handler(bool in_handler)
    {
      m_in_signal_handler = in_handler;
    }
    inline bool thread_local_kernel_state_t::in_signal_handler() const
    {
      return m_in_signal_handler;
    }
    inline ::std::thread::native_handle_type thread_local_kernel_state_t::thread_handle() const
    {
      return m_thread_handle;
    }
    inline ::std::thread::id thread_local_kernel_state_t::thread_id() const
    {
      return m_thread_id;
    }
    inline void thread_local_kernel_state_t::add_potential_root(void *root)
    {
      if (m_potential_roots.size() == m_potential_roots.capacity())
        throw ::std::runtime_error("CGC1 TLKS potential roots full");
      m_potential_roots.push_back(root);
    }
    inline void thread_local_kernel_state_t::clear_potential_roots()
    {
      m_potential_roots.clear();
    }
    inline const cgc_internal_vector_t<void *> &thread_local_kernel_state_t::_potential_roots() const
    {
      return m_potential_roots;
    }
    template <typename CONTAINER>
    void thread_local_kernel_state_t::scan_stack(
        CONTAINER &container, uint8_t *ibegin, uint8_t *iend, uint8_t *fast_slab_begin, uint8_t *fast_slab_end)
    {
      // can't recover if we didn't set stack ptrs.
      if (!static_cast<uint8_t *>(m_stack_ptr) || !static_cast<uint8_t *>(m_top_of_stack))
        abort();
      uint8_t **unaligned = reinterpret_cast<uint8_t **>(m_stack_ptr.load());
      uint8_t **stack_ptr = ::mcppalloc::align_pow2(unaligned, 3);
      assert(unaligned == stack_ptr);
      // crawl stack looking for addresses between ibegin and iend.
      for (uint8_t **v = stack_ptr; v != reinterpret_cast<uint8_t **>(m_top_of_stack); ++v) {
        void *os = gc_sparse_object_state_t::from_object_start(*v);
        if (os >= ibegin && os < iend) {
          container.emplace_back(v);
        }
        else if (*v >= fast_slab_begin && *v < fast_slab_end) {
          container.emplace_back(v);
        }
      }
    }
  }
}
