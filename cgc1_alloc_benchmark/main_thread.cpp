#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>
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
#include <cgc1/cgc1.hpp>
#include <cgc1/posix.hpp>
#include <csignal>
#include <mcppalloc/mcppalloc_sparse/allocator.hpp>
#include <thread>
#endif

static const size_t num_alloc = 10000000;
static const size_t num_thread = ::std::thread::hardware_concurrency() * 4;
static const size_t num_thread_alloc = num_alloc / num_thread;
static const size_t alloc_sz = 64;
static ::std::atomic<bool> go{false};
static ::std::atomic<size_t> done{0};

static ::std::condition_variable done_cv;
static ::std::condition_variable start_cv;
static ::std::mutex go_mutex;
static ::std::mutex cv_mutex;

void thread_main()
{
  ::std::vector<void *> ptrs;
#ifdef BOEHM
  GC_stack_base sb;
  GC_get_stack_base(&sb);
  GC_register_my_thread(&sb);
  ::std::unique_lock<::std::mutex> go_lk(go_mutex);
  start_cv.wait(go_lk, []() { return go.load(); });
  go_lk.unlock();
  for (size_t i = 0; i < num_thread_alloc; ++i) {
    auto ret = GC_malloc(alloc_sz);
    if (ret == nullptr) {
      ::std::terminate();
    }
    ptrs.emplace_back(ret);
  }
  for (size_t i = 0; i < ptrs.size(); ++i) {
    if ((i % 2) != 0u) {
      GC_free(ptrs[i]);
    }
  }
  ptrs.clear();
  for (size_t i = 0; i < num_thread_alloc / 2; ++i) {
    auto ret = GC_malloc(alloc_sz);
    if (ret == nullptr) {
      ::std::terminate();
    }
    ptrs.emplace_back(ret);
  }
  ++done;
  done_cv.notify_all();
  GC_unregister_my_thread();
#elif defined(SYSTEM_MALLOC)
  ::std::unique_lock<::std::mutex> go_lk(go_mutex);
  start_cv.wait(go_lk, []() { return go.load(); });
  go_lk.unlock();
  for (size_t i = 0; i < num_thread_alloc; ++i) {
    auto ret = malloc(alloc_sz);
    if (ret == nullptr) {
      ::std::terminate();
    }
    ptrs.emplace_back(ret);
  }
  for (size_t i = 0; i < ptrs.size(); ++i) {
    if ((i % 2) != 0u) {
      free(ptrs[i]);
    }
  }
  ptrs.clear();
  for (size_t i = 0; i < num_thread_alloc / 2; ++i) {
    auto ret = malloc(alloc_sz);
    if (ret == nullptr) {
      ::std::terminate();
    }
    ptrs.emplace_back(ret);
  }
  ++done;
  done_cv.notify_all();
#else
  CGC1_INITIALIZE_THREAD();
#ifdef CGC1_SPARSE
  auto &allocator = cgc1::details::g_gks->gc_allocator();
  auto &ts = allocator.initialize_thread();

  using ts_type = typename ::std::decay<decltype(ts)>::type;
  for (size_t i = 0; i < ts_type::c_bins; ++i) {
    ts.set_allocator_multiple(i, ts.get_allocator_multiple(i) * 256);
  }
#else
  auto &ts = cgc1::details::g_gks->_bitmap_allocator().initialize_thread();
#endif
  ::std::unique_lock<::std::mutex> go_lk(go_mutex);
  start_cv.wait(go_lk, []() { return go.load(); });
  go_lk.unlock();

  for (size_t i = 0; i < num_thread_alloc; ++i) {
    auto ret = ts.allocate(alloc_sz).m_ptr;
    if (nullptr == ret) {
      {
        ::std::terminate();
      }
    }
    ptrs.emplace_back(ret);
  }
  for (size_t i = 0; i < ptrs.size(); ++i) {
    if (static_cast<unsigned int>((i % 2) != 0u)) {
      {
        ts.deallocate(ptrs[i]);
      }
    }
  }
  ptrs.clear();
  for (size_t i = 0; i < num_thread_alloc / 2; ++i) {
    auto ret = ts.allocate(alloc_sz).m_ptr;
    if (ret == nullptr) {
      {
        ::std::terminate();
      }
    }
    ptrs.emplace_back(ret);
  }
  ++done;
  cv_mutex.lock();
  done_cv.notify_all();
  cv_mutex.unlock();
  cgc1::cgc_unregister_thread();
#endif
}

int main()
{
#ifdef BOEHM
  ::std::cout << "Using bohem\n";
  GC_INIT();
  GC_disable();
  GC_allow_register_threads();
#elif defined(SYSTEM_MALLOC)
  ::std::cout << "System malloc\n";
#else
#ifdef CGC1_SPARSE
  ::std::cout << "Using cgc1 sparse\n";
#else
  ::std::cout << "Using cgc1\n";
#endif
  CGC1_INITIALIZE_THREAD();
#endif
  ::std::vector<::std::thread> threads;
  for (size_t i = 0; i < num_thread; ++i) {
    threads.emplace_back(thread_main);
  }

  ::std::chrono::high_resolution_clock::time_point t1 = ::std::chrono::high_resolution_clock::now();
  ::std::unique_lock<::std::mutex> go_lk(go_mutex);
  go = true;
  start_cv.notify_all();
  go_lk.unlock();
  ::std::unique_lock<::std::mutex> cv_lk(cv_mutex);
  done_cv.wait(cv_lk, []() { return done == num_thread; });
  ::std::chrono::high_resolution_clock::time_point t2 = ::std::chrono::high_resolution_clock::now();
  cv_lk.unlock();
  assert(done == num_thread);
  ::std::chrono::duration<double> time_span = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - t1);
  ::std::cout << "Time elapsed: " << time_span.count() << ::std::endl;
  for (auto &&thread : threads) {
    {
      {
        {
          thread.join();
        }
      }
    }
  }
  t1 = ::std::chrono::high_resolution_clock::now();
#ifdef BOEHM
  GC_enable();
  GC_gcollect();
#elif defined(SYSTEM_MALLOC)
#else
  cgc1::cgc_force_collect();
#endif
  t2 = ::std::chrono::high_resolution_clock::now();
  time_span = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - t1);
  ::std::cout << "Time elapsed: " << time_span.count() << ::std::endl;
#if defined(BOEHM) || defined(SYSTEM_MALLOC)
#else
  ::cgc1::cgc_shutdown();
#endif
  return 0;
}
