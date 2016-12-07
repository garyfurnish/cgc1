#include <cgc1/posix.hpp>
#ifdef MCPPALLOC_POSIX
#include <array>
#include <csignal>
#include <iostream>
#include <mcpputil/mcpputil/posix_slab.hpp>
#include <pthread.h>
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
  bool register_signal_handler(int signum, const signal_handler_function_t &handler)
  {
    const auto isignum = signum;
    if (isignum < 0) {
      {
        ::std::abort();
      }
    }
    auto ssignum = static_cast<size_t>(isignum);
    if (ssignum >= SIGUNUSED) {
      return false;
    }
    if (handler) {
      details::signal_handler_functions()[ssignum] = handler;
      signal(isignum, [](int lsignum) { details::signal_handler_functions()[static_cast<size_t>(lsignum)](lsignum); });
    } else {
      signal(isignum, SIG_IGN);
      details::signal_handler_functions()[ssignum] = handler;
    }
    return true;
  }
  void set_default_signal_handler(int signum)
  {
    if (signum < 0) {
      ::std::abort();
    }
    auto ssignum = static_cast<size_t>(signum);
    signal(signum, SIG_DFL);
    details::signal_handler_functions()[ssignum] = nullptr;
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
