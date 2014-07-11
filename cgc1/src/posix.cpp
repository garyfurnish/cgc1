#include <cgc1/posix.hpp>
#ifdef CGC1_POSIX
#include <pthread.h>
#include <signal.h>
#include <thread>
#include <iostream>
#include <cgc1/posix_slab.hpp>
namespace cgc1
{
  size_t slab_t::s_page_size = 0;
  namespace details
  {
    extern void thread_gc_handler(int);
    void suspension(int sig)
    {
      thread_gc_handler(sig);
    }
    void initialize_thread_suspension()
    {
      register_signal_handler(SIGUSR1, suspension);
    }
    static ::std::function<void(int)> s_signal_handlers[SIGUNUSED];
  }
  bool register_signal_handler(int signum, const signal_handler_function_t &handler)
  {
    if (signum >= SIGUNUSED) {
      return false;
    }
    if (handler) {
      details::s_signal_handlers[signum] = handler;
      signal(signum, [](int signum) { details::s_signal_handlers[signum](signum); });
    } else {
      signal(signum, SIG_IGN);
      details::s_signal_handlers[signum] = handler;
    }
    return true;
  }
  void set_default_signal_handler(int signum)
  {
    signal(signum, SIG_DFL);
    details::s_signal_handlers[signum] = nullptr;
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