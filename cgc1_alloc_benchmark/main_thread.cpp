#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#ifdef BOEHM
#define GC_THREADS
#include <gc.h>
#else
#include <cgc1/gc.h>

#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include <cgc1/posix_slab.hpp>
#include <cgc1/posix.hpp>
#include <thread>
#include <signal.h>
#include "../cgc1/src/allocator_block.hpp"
#include "../cgc1/src/allocator.hpp"
#include "../cgc1/src/global_kernel_state.hpp"
#endif

//static const size_t num_alloc = 10000000;
static const size_t num_alloc = 6000000;
static const size_t num_thread = 20;
static const size_t num_thread_alloc = num_alloc/num_thread;
static const size_t alloc_sz = 64;
static ::std::atomic<bool> go{false};
static ::std::atomic<int> done{0};

void thread_main()
{
  #ifdef BOEHM
  GC_stack_base sb;
  GC_get_stack_base(&sb);
  GC_register_my_thread(&sb);
  while(!go)
    ;
  for (size_t i = 0; i < num_thread_alloc; ++i) {
    auto ret = GC_malloc(alloc_sz);
    if (!ret)
      abort();
  }
  ++done;
  GC_unregister_my_thread();
  #else
  auto& allocator = cgc1::details::g_gks.gc_allocator();
  auto &ts = allocator.initialize_thread();
  while(!go)
    ;
  for (size_t i = 0; i < num_thread_alloc; ++i) {
    if(!ts.allocate(alloc_sz))
      abort();
  }
  ++done;
  #endif
}

int main()
{
#ifdef BOEHM
  ::std::cout << "Using bohem\n";
  GC_disable();
  GC_INIT();  
  GC_allow_register_threads();
#else
  ::std::cout << "Using cgc1\n";
  CGC1_INITIALIZE_THREAD();
#endif
  ::std::vector<::std::thread> threads;
  for(size_t i = 0; i < num_thread; ++i)
    {
      threads.emplace_back(thread_main);
    }
  ::std::chrono::high_resolution_clock::time_point t1 = ::std::chrono::high_resolution_clock::now();
  go = true;
  while(done!=num_thread)
    ;
  ::std::chrono::high_resolution_clock::time_point t2 = ::std::chrono::high_resolution_clock::now();
  ::std::chrono::duration<double> time_span = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - t1);
  ::std::cout << "Time elapsed: " << time_span.count() << ::std::endl;
  for(auto&& thread : threads)
    thread.join();
  t1 = ::std::chrono::high_resolution_clock::now();
  #ifdef BOEHM
  GC_enable();
  GC_gcollect();
  #else
  cgc1::cgc_force_collect();
  #endif
  t2 = ::std::chrono::high_resolution_clock::now();
  time_span = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - t1);
  ::std::cout << "Time elapsed: " << time_span.count() << ::std::endl;

  return 0;
}
