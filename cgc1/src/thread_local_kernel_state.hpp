#pragma once
#include "gc_allocator.hpp"
#include "internal_allocator.hpp"
#include "internal_declarations.hpp"
#include <atomic>
#include <cassert>
#include <cgc1/declarations.hpp>
#include <mcppalloc/mcppalloc_bitmap_allocator/bitmap_allocator.hpp>
#include <thread>
#include <vector>
namespace cgc1
{
  namespace details
  {
    /**
     * \brief Store thread local gc kernel state.
     **/
    class thread_local_kernel_state_t
    {
    public:
      using bitmap_thread_allocator_type =
          typename ::mcppalloc::bitmap_allocator::bitmap_allocator_t<gc_bitmap_allocator_policy_t>::thread_allocator_type;
      /**
       * \brief Constructor.
       **/
      thread_local_kernel_state_t();
      thread_local_kernel_state_t(const thread_local_kernel_state_t &) = delete;
      thread_local_kernel_state_t(thread_local_kernel_state_t &&) = delete;
      thread_local_kernel_state_t &operator=(const thread_local_kernel_state_t &) = delete;
      thread_local_kernel_state_t &operator=(thread_local_kernel_state_t &&) = delete;
      /**
       * \brief Destructor.
       **/
      ~thread_local_kernel_state_t();
      /**
       * \brief Return top of stack for this thread.
      **/
      uint8_t *top_of_stack() const;
      /**
       * \brief Set top of stack for this thread.
      **/
      void set_top_of_stack(void *top);
      /**
       * \brief Set current stack pointer for this thread.
       *
       * Only has meaning inside of garbage collection.
      **/
      void set_stack_ptr(void *stack_ptr);
      /**
       * \brief Return the current stack pointer for this thread.
       *
       * Only has meaning inside of garbage collection.
      **/
      uint8_t *current_stack_ptr() const;
      /**
       * \brief Set if this thread is in its signal handler.
      **/
      void set_in_signal_handler(bool in_handler);
      /**
       * \brief Return true if this thread is in its signal handler, false otherwise.
       *
       * The exact meaning is defined by gks.
       * It may be true while technically in signal handler if currently exiting.
      **/
      bool in_signal_handler() const;
      /**
       * \brief Return the native thread handle for this thread.
      **/
      ::std::thread::native_handle_type thread_handle() const;
      /**
       * \brief Return the thread id for this thread.
      **/
      ::std::thread::id thread_id() const;
      /**
      * \brief Scan stack for addresses between begin and end.
      *
      * @param container Destination for addresses found.
      * @param begin Start of potential addresses.
      * @param end End of potential addresses.
      * @param fast_slab_begin Beginning of fast slab.
      * @param fast_slab_end End of fast slab.
      **/
      template <typename CONTAINER>
      void scan_stack(
          CONTAINER &container, uint8_t *begin, uint8_t *end, uint8_t *const fast_slab_begin, uint8_t *const fast_slab_end);
      /**
      * \brief Add a location that may potentially hold a root.
      *
      * This is typically used to hold registers.
      * @param root Potential root, may be nullptr.
      **/
      void add_potential_root(void *root);
      /**
      * \brief Clear all potential roots.
      **/
      void clear_potential_roots();
      /**
       * \brief Return list of potential roots.
       **/
      const cgc_internal_vector_t<void *> &_potential_roots() const;
      /**
       * \brief Return sparse thread allocator.
       **/
      auto thread_allocator() const noexcept -> typename gc_allocator_t::this_thread_allocator_t *;
      /**
       * \brief Set sparse thread allocator.
       **/
      void set_thread_allocator(typename gc_allocator_t::this_thread_allocator_t *allocator);
      /**
       * \brief Return bitmap thread allocator.
       **/
      auto bitmap_thread_allocator() const noexcept -> bitmap_thread_allocator_type *;
      /**
       * \brief Set bitmap thread allocator.
       **/
      void set_bitmap_thread_allocator(bitmap_thread_allocator_type *allocator);

    private:
      /**
       * \brief Cached sparse thread allocator.
       **/
      typename gc_allocator_t::this_thread_allocator_t *m_thread_allocator = nullptr;
      /**
       * \brief Cached bitmap thread allocator.
       **/
      bitmap_thread_allocator_type *m_bitmap_thread_allocator = nullptr;
      /**
      * \brief Native thread handle for this thread.
      **/
      ::std::thread::native_handle_type m_thread_handle;
      /**
      * \brief Thread id for this thread.
      **/
      ::std::thread::id m_thread_id;
      /**
      * \brief Top of stack for this thread.
      **/
      uint8_t *m_top_of_stack{nullptr};
      /**
      * \brief Current stack pointer.
      **/
      ::std::atomic<uint8_t *> m_stack_ptr{nullptr};
      /**
      * \brief True if in signal handler for this thread, false otherwise.
      **/
      ::std::atomic<bool> m_in_signal_handler{false};
      /**
      * \brief List of other potential roots.
      *
      * This is typically used to hold registers on machines that do not push them onto the stack.
      **/
      cgc_internal_vector_t<void *> m_potential_roots;
    };
  }
}
#include "thread_local_kernel_state_impl.hpp"
