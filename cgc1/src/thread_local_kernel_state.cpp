#include "thread_local_kernel_state.hpp"
#include "object_state.hpp"
#include "allocator.hpp"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace cgc1
{
  namespace details
  {
#ifdef WIN32
    thread_local_kernel_state_t::thread_local_kernel_state_t()
        : m_thread_id(::std::this_thread::get_id()), m_stack_ptr(nullptr), m_in_signal_handler(false)
    {
      auto lth = ::GetCurrentThread();
      DuplicateHandle(GetCurrentProcess(), lth, GetCurrentProcess(), &m_thread_handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
      assert(m_thread_handle);
    }
    thread_local_kernel_state_t::~thread_local_kernel_state_t()
    {
      CloseHandle(m_thread_handle);
    }
    void thread_local_kernel_state_t::collect_current_thread()
    {
    }
#else
    thread_local_kernel_state_t::thread_local_kernel_state_t()
        : m_thread_id(::std::this_thread::get_id()), m_stack_ptr(nullptr), m_in_signal_handler(false)
    {
      m_thread_handle = pthread_self();
      assert(m_thread_handle);
    }
    thread_local_kernel_state_t::~thread_local_kernel_state_t()
    {
    }
    void thread_local_kernel_state_t::collect_current_thread()
    {
    }
#endif
  }
}
