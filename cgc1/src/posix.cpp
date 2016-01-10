#include <cgc1/posix.hpp>
#ifdef MCPPALLOC_POSIX
#include <array>
#include <iostream>
#include <mcppalloc/mcppalloc_utils/posix_slab.hpp>
#include <pthread.h>
#include <signal.h>
#include <thread>
namespace cgc1
{
  namespace details
  {
    extern void thread_gc_handler(int);
    static void suspension(int sig)
    {
      thread_gc_handler(sig);
    }
    void initialize_thread_suspension()
    {
      register_signal_handler(SIGUSR1, suspension);
    }
    void stop_thread_suspension()
    {
    }
    using function_array_type = ::std::array<::std::function<void(int)>, SIGUNUSED>;
    static function_array_type &signal_handler_functions()
    {
      static function_array_type s_signal_handlers;
      return s_signal_handlers;
    }
  }
  bool register_signal_handler(int isignum, const signal_handler_function_t &handler)
  {
    if (isignum < 0)
      ::std::abort();
    auto signum = static_cast<size_t>(isignum);
    if (signum >= SIGUNUSED) {
      return false;
    }
    if (handler) {
      details::signal_handler_functions()[signum] = handler;
      signal(isignum, [](int lsignum) { details::signal_handler_functions()[static_cast<size_t>(lsignum)](lsignum); });
    } else {
      signal(isignum, SIG_IGN);
      details::signal_handler_functions()[signum] = handler;
    }
    return true;
  }
  void set_default_signal_handler(int isignum)
  {
    if (isignum < 0)
      ::std::abort();
    auto signum = static_cast<size_t>(isignum);
    signal(isignum, SIG_DFL);
    details::signal_handler_functions()[signum] = nullptr;
  }
  int pthread_kill(::std::thread &thread, int sig)
  {
    pthread_t pt = thread.native_handle();
    return cgc1::pthread_kill(pt, sig);
  }
  int pthread_kill(const ::std::thread::native_handle_type &thread, int sig)
  {
    return ::pthread_kill(thread, sig);
  }
}
#endif
