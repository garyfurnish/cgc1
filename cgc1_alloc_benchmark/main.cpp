#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>
#ifdef BOEHM
#define GC_THREADS
#include <gc.h>
#ifdef CGC1_FAKE_BOEHM
#error
#endif
#else
#include <cgc1/gc.h>

#include "../cgc1/src/global_kernel_state.hpp"
#include "../cgc1/src/internal_declarations.hpp"
#include "../cgc1/src/ptree.hpp"
#include <cgc1/cgc1.hpp>
#include <cgc1/posix.hpp>
#include <csignal>
#include <mcppalloc/mcppalloc_sparse/allocator.hpp>
#include <thread>

#endif

int main()
{
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
    if (ret == nullptr) {
      ::std::terminate();
    }
    for (size_t li = 0; li < 8; ++li) {
      ret[li] = GC_malloc(alloc_sz);
    }

    ptrs[i] = ret;
  }
#elif defined(CGC1_SPARSE)
  ::std::cout << "Using cgc1 sparse\n";
  CGC1_INITIALIZE_THREAD();
  auto &allocator = cgc1::details::g_gks->gc_allocator();
  ::std::cout << ::cgc1::to_json(*cgc1::details::g_gks, 2) << ::std::endl;

  auto &ts = allocator.initialize_thread();
  ::std::cout << ts.to_json(2) << ::std::endl;
  for (size_t i = 0; i < num_alloc; ++i) {
    auto ret = reinterpret_cast<void **>(ts.allocate(alloc_sz).m_ptr);
    if (ret == nullptr) {
      ::std::terminate();
    }
    for (size_t li = 0; li < 8; ++li) {
      ret[li] = ts.allocate(alloc_sz).m_ptr;
    }
    ::mcpputil::secure_zero(&ret, sizeof(ret));
  }

#else
  ::std::cout << "Using cgc1\n";
  CGC1_INITIALIZE_THREAD();
  auto &allocator = cgc1::details::g_gks->_bitmap_allocator();

  auto &ts = allocator.initialize_thread();
  for (size_t i = 0; i < num_alloc; ++i) {
    auto ret = reinterpret_cast<void **>(ts.allocate(alloc_sz).m_ptr);
    if (ret == nullptr) {
      ::std::terminate();
    }
    for (size_t li = 0; li < 8; ++li) {
      if ((ret[li] = ts.allocate(alloc_sz).m_ptr) == nullptr) {
        ::std::terminate();
      }
    }
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
  ::std::cout << cgc1::to_json(*cgc1::details::g_gks, 2) << ::std::endl;
  auto &int_ts = cgc1::details::g_gks->_internal_allocator().initialize_thread();
  ts._do_maintenance();
  int_ts._do_maintenance();
  ts.shrink_secondary_memory_usage_to_fit();
  ::std::cout << cgc1::to_json(*cgc1::details::g_gks, 2) << ::std::endl;
#else
  cgc1::cgc_force_collect();
  ::std::cout << cgc1::to_json(*cgc1::details::g_gks, 2) << ::std::endl;
#endif
  hrc::time_point t3 = hrc::now();
  ::std::chrono::duration<double> setup = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - t1);
  ::std::chrono::duration<double> teardown = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t3 - t2);
  ::std::cout << "Setup Time elapsed: " << setup.count() << ::std::endl;
  ::std::cout << "Teardown Time elapsed: " << teardown.count() << ::std::endl;
#ifndef BOEHM
  ::cgc1::cgc_shutdown();
#endif
  return 0;
}
