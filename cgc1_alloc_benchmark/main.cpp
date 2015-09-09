#include <iostream>
#include <chrono>
#include <vector>
#include <cstring>
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
  //  const size_t num_alloc = 100000000;
  const size_t num_alloc = 10000000;
  const size_t alloc_sz = 64;

  using hrc = ::std::chrono::high_resolution_clock;
  hrc::time_point t1 = hrc::now();
  void **ptrs = nullptr;
  (void)ptrs;
#ifdef BOEHM
  ::std::cout << "Using bohem\n";
  GC_disable();
  GC_INIT();
  ptrs = reinterpret_cast<void **>(GC_malloc(sizeof(void *) * num_alloc));
  for (size_t i = 0; i < num_alloc; ++i) {
    auto ret = reinterpret_cast<void **>(GC_malloc(alloc_sz));
    if (!ret)
      abort();
    for (size_t li = 0; li < 8; ++li) {
      ret[li] = GC_malloc(alloc_sz);
    }

    ptrs[i] = ret;
  }
#elif defined(CGC1_SPARSE)
  ::std::cout << "Using cgc1 sparse\n";
  CGC1_INITIALIZE_THREAD();
  auto &allocator = cgc1::details::g_gks.gc_allocator();

  auto &ts = allocator.initialize_thread();
  for (size_t i = 0; i < num_alloc; ++i) {
    auto ret = reinterpret_cast<void **>(ts.allocate(alloc_sz));
    if (!ret)
      abort();
    for (size_t li = 0; li < 8; ++li)
      ret[li] = ts.allocate(alloc_sz);
  }

#else
  ::std::cout << "Using cgc1\n";
  CGC1_INITIALIZE_THREAD();
  auto &allocator = cgc1::details::g_gks._packed_object_allocator();

  auto &ts = allocator.initialize_thread();
  for (size_t i = 0; i < num_alloc; ++i) {
    auto ret = reinterpret_cast<void **>(ts.allocate(alloc_sz));
    if (!ret)
      abort();
    for (size_t li = 0; li < 8; ++li)
      ret[li] = ts.allocate(alloc_sz);
  }
#endif
  hrc::time_point t2 = hrc::now();
#ifdef BOEHM
  GC_enable();
  ::std::cout << GC_base(ptrs) << ::std::endl;
  ::std::cout << "Free bytes " << GC_get_free_bytes() << ::std::endl;
  ::std::cout << "heap size is " << GC_get_heap_size() << ::std::endl;
  ::std::cout << "Total bytes " << GC_get_total_bytes() << ::std::endl;
  ::std::cout << "Unampped bytes " << GC_get_unmapped_bytes() << ::std::endl;

  GC_gcollect();
  ::std::cout << "Collected\n";
  ::std::cout << "Free bytes " << GC_get_free_bytes() << ::std::endl;
  ::std::cout << "heap size is " << GC_get_heap_size() << ::std::endl;
  ::std::cout << "Total bytes " << GC_get_total_bytes() << ::std::endl;
  ::std::cout << "Unampped bytes " << GC_get_unmapped_bytes() << ::std::endl;
#elif defined(CGC1_SPARSE)
  cgc1::cgc_force_collect();
  auto &gks = cgc1::details::g_gks;
  ::std::cout << "Clear: " << gks.clear_mark_time_span().count() << ::std::endl;
  ::std::cout << "Mark: " << gks.mark_time_span().count() << ::std::endl;
  ::std::cout << "Sweep: " << gks.sweep_time_span().count() << ::std::endl;
  ::std::cout << "Notify: " << gks.notify_time_span().count() << ::std::endl;
  ::std::cout << gks.total_collect_time_span().count() << ::std::endl;

#else
  cgc1::cgc_force_collect();
  auto &gks = cgc1::details::g_gks;
  ::std::cout << "Clear: " << gks.clear_mark_time_span().count() << ::std::endl;
  ::std::cout << "Mark: " << gks.mark_time_span().count() << ::std::endl;
  ::std::cout << "Sweep: " << gks.sweep_time_span().count() << ::std::endl;
  ::std::cout << "Notify: " << gks.notify_time_span().count() << ::std::endl;
  ::std::cout << gks.total_collect_time_span().count() << ::std::endl;

#endif
  hrc::time_point t3 = hrc::now();
  ::std::chrono::duration<double> setup = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - t1);
  ::std::chrono::duration<double> teardown = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t3 - t2);
  ::std::cout << "Setup Time elapsed: " << setup.count() << ::std::endl;
  ::std::cout << "Teardown Time elapsed: " << teardown.count() << ::std::endl;
  return 0;
}
