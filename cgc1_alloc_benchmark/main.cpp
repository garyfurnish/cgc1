#include <iostream>
#include <chrono>
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

int main()
{
  const size_t num_alloc = 100000000;
  const size_t alloc_sz = 64;
  ::std::chrono::high_resolution_clock::time_point t1 = ::std::chrono::high_resolution_clock::now();
#ifdef BOEHM
  ::std::cout << "Using bohem\n";
  GC_disable();
  GC_INIT();
  for (size_t i = 0; i < num_alloc; ++i) {
    auto ret = GC_malloc(alloc_sz);
    if (!ret)
      abort();
  }
#else
  ::std::cout << "Using cgc1\n";
  CGC1_INITIALIZE_THREAD();
  auto &allocator = cgc1::details::g_gks.gc_allocator();

  auto &ts = allocator.initialize_thread();
  for (size_t i = 0; i < num_alloc; ++i) {
    ts.allocate(alloc_sz);
  }
#endif
  ::std::chrono::high_resolution_clock::time_point t2 = ::std::chrono::high_resolution_clock::now();
  ::std::chrono::duration<double> time_span = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - t1);
  ::std::cout << "Time elapsed: " << time_span.count() << ::std::endl;
  return 0;
}
