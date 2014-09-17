#pragma once
#include "internal_declarations.hpp"
#include <cgc1/declarations.hpp>
#include <atomic>
#include <thread>
#include <assert.h>
namespace cgc1
{
  namespace details
  {
    class thread_local_kernel_state_t
    {
    public:
      thread_local_kernel_state_t();
      thread_local_kernel_state_t(const thread_local_kernel_state_t &) = delete;
      thread_local_kernel_state_t(thread_local_kernel_state_t &&) = default;
      thread_local_kernel_state_t &operator=(const thread_local_kernel_state_t &) = delete;
      thread_local_kernel_state_t &operator=(thread_local_kernel_state_t &&) = default;
      ~thread_local_kernel_state_t();
      /**
      Return top of stack for this thread.
      **/
      uint8_t *top_of_stack() const;
      /**
      Set top of stack for this thread.
      **/
      void set_top_of_stack(void *top);
      /**
      Set current stack pointer for this thread.
      Only has meaning inside of garbage collection.
      **/
      void set_stack_ptr(void *stack_ptr);
      /**
      Return the current stack pointer for this thread.
      Only has meaning inside of garbage collection.
      **/
      uint8_t *current_stack_ptr() const;
      /**
      Set if this thread is in its signal handler.
      **/
      void set_in_signal_handler(bool in_handler);
      /**
      Return true if this thread is in its signal handler, false otherwise.
      **/
      bool in_signal_handler() const;
      /**
      Return the native thread handle for this thread.
      **/
      ::std::thread::native_handle_type thread_handle() const;
      /**
      Return the thread id for this thread.
      **/
      ::std::thread::id thread_id() const;
      /**
      Scan stack for addresses between begin and end.
      **/
      template <typename CONTAINER>
      void scan_stack(CONTAINER &container, uint8_t *begin, uint8_t *end);

    private:
      /**
      Native thread handle for this thread.
      **/
      ::std::thread::native_handle_type m_thread_handle;
      /**
      Thread id for this thread.
      **/
      ::std::thread::id m_thread_id;
      /**
      Top of stack for this thread.
      **/
      uint8_t *m_top_of_stack = nullptr;
      /**
      Current stack pointer.
      **/
      ::std::atomic<uint8_t *> m_stack_ptr;
      /**
      True if in signal handler for this thread, false otherwise.
      **/
      ::std::atomic<bool> m_in_signal_handler;
    };
  }
}
#include "thread_local_kernel_state_impl.hpp"
