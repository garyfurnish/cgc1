#pragma once
#include "declarations.hpp"
#ifdef MCPPALLOC_POSIX
#include <functional>
#include <thread>
namespace cgc1
{
  using signal_handler_function_t = typename ::std::function<void(int)>;
  /**
   * \brief Register a signal handler function with the signal handler.
   *
   * @param signum Signal to register
   * @param handler Function to register.
   * @return True on success, false otherwise.
  **/
  extern bool register_signal_handler(int signum, const signal_handler_function_t &handler);
  /**
   * \brief Set the given signal handler to the default handler.
   *
   * @param signum Signal to register.
  **/
  extern void set_default_signal_handler(int signum);
  /**
   * \brief Send a signal to a thread.
   *
   * @param thread Thread to signal.
   * @param sig Signal to send.
  **/
  extern int pthread_kill(::std::thread &thread, int sig);
  /**
   * \brief Send a signal to a thread.
   *
   * @param thread Thread to signal.
   * @param sig Signal to send.
  **/
  extern int pthread_kill(const ::std::thread::native_handle_type &thread, int sig);
  namespace details
  {
    /**
     * \brief Initialize gc handling using USR1.
    **/
    void initialize_thread_suspension();
    /**
     * \brief Stop handling gc with USR1.
     **/
    void stop_thread_suspension();
  }
}
#endif
