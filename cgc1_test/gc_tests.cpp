#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include "bandit.hpp"
#include <cgc1/posix_slab.hpp>
#include <cgc1/posix.hpp>
#include <cgc1/aligned_allocator.hpp>
#include "../cgc1/src/slab_allocator.hpp"
#include <thread>
#include <chrono>
#include <string.h>
#include <signal.h>
#include "../cgc1/src/allocator_block.hpp"
#include "../cgc1/src/allocator.hpp"
#include "../cgc1/src/internal_allocator.hpp"
#include "../cgc1/src/global_kernel_state.hpp"
#include "../cgc1/src/internal_stream.hpp"
#include "../cgc1/src/packed_object_state.hpp"
#include "../cgc1/src/packed_object_allocator.hpp"
static ::std::vector<size_t> locations;
static cgc1::spinlock_t debug_mutex;
using namespace bandit;
// alias
static auto &gks = ::cgc1::details::g_gks;
using namespace ::cgc1::literals;

/**
 * \brief Setup for root test.
 * This must be a separate funciton to make sure the compiler does not hide pointers somewhere.
 **/
static _NoInline_ void root_test__setup(void *&memory, size_t &old_memory)
{
  memory = cgc1::cgc_malloc(50);
  // hide a pointer away for comparison testing.
  old_memory = cgc1::hide_pointer(memory);
  cgc1::cgc_add_root(&memory);
  AssertThat(cgc1::cgc_size(memory), Equals(static_cast<size_t>(64)));
  AssertThat(cgc1::cgc_is_cgc(memory), IsTrue());
  AssertThat(cgc1::cgc_is_cgc(nullptr), IsFalse());
}
/**
 * \brief Test root functionality.
 **/
static void root_test()
{
  void *memory;
  size_t old_memory;
  // setup a root.
  root_test__setup(memory, old_memory);
  // force collection
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
  // verify that nothing was collected.
  auto last_collect = gks._d_freed_in_last_collection();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  AssertThat(last_collect, HasLength(0));
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(0_sz));
  // remove the root.
  cgc1::cgc_remove_root(&memory);
  // make sure that the we zero the memory so the pointer doesn't linger.
  cgc1::secure_zero_pointer(memory);
  auto num_collections = cgc1::debug::num_gc_collections();
  // force collection.
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
  last_collect = gks._d_freed_in_last_collection();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  // now we should collect.
  AssertThat(last_collect.size(), Equals(1_sz));
  // verify it collected the correct address.
  AssertThat(last_collect[0] == old_memory, IsTrue());
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(1_sz));
  // verify that we did perform a collection.
  AssertThat(cgc1::debug::num_gc_collections(), Equals(num_collections + 1));
}
/**
 * \brief Setup for internal pointer test.
 **/
static _NoInline_ void internal_pointer_test__setup(void *&memory, size_t &old_memory)
{
  memory = cgc1::cgc_malloc(50);
  uint8_t *&umemory = cgc1::unsafe_reference_cast<uint8_t *>(memory);
  old_memory = cgc1::hide_pointer(memory);
  umemory += 1;
  cgc1::cgc_add_root(&memory);
}
/**
 * \brief This tests pointers pointing to the inside of an object as opposed to the start.
 **/
static void internal_pointer_test()
{
  void *memory;
  size_t old_memory;
  // setup a buffer to point into.
  internal_pointer_test__setup(memory, old_memory);
  // force collection.
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto last_collect = gks._d_freed_in_last_collection();
  // it should stick around because it has a root.
  AssertThat(last_collect, HasLength(0));
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(0_sz));
  // remove the root.
  cgc1::cgc_remove_root(&memory);
  // make sure that we zero the memory so the pointer doesn't linger.
  cgc1::secure_zero_pointer(memory);
  // force collection.
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  last_collect = gks._d_freed_in_last_collection();
  // now we should collect.
  AssertThat(last_collect, HasLength(1));
  // verify it collected the correct address.
  AssertThat(last_collect[0] == old_memory, IsTrue());
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(1_sz));
}
/**
 * \brief Setup for atomic object test.
 **/
static _NoInline_ void atomic_test__setup(void *&memory, size_t &old_memory)
{
  memory = cgc1::cgc_malloc(50);
  cgc1::cgc_add_root(&memory);
  void *memory2 = cgc1::cgc_malloc(50);
  *reinterpret_cast<void **>(memory) = memory2;
  old_memory = cgc1::hide_pointer(memory2);
  cgc1::secure_zero_pointer(memory2);
}
/**
 * \brief Test atomic object functionality.
 **/
static void atomic_test()
{
  void *memory;
  size_t old_memory;
  atomic_test__setup(memory, old_memory);
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto last_collect = gks._d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(0_sz));
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(0_sz));
  cgc1::cgc_set_atomic(memory, true);
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  last_collect = gks._d_freed_in_last_collection();

  AssertThat(last_collect, HasLength(1));
  AssertThat(last_collect[0] == old_memory, IsTrue());
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(1_sz));
  cgc1::cgc_remove_root(&memory);
  cgc1::secure_zero_pointer(memory);
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  AssertThat(last_collect, HasLength(1));
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(1_sz));
  // test bad parameters
  cgc1::cgc_set_atomic(nullptr, true);
  cgc1::cgc_set_atomic(&old_memory, true);
}
static _NoInline_ void finalizer_test__setup(std::atomic<bool> &finalized, size_t &old_memory)
{
  void *memory = cgc1::cgc_malloc(50);
  old_memory = cgc1::hide_pointer(memory);
  finalized = false;
  cgc1::cgc_register_finalizer(memory, [&finalized](void *) { finalized = true; });
  cgc1::secure_zero_pointer(memory);
}
static void finalizer_test()
{
  size_t old_memory;
  std::atomic<bool> finalized;
  finalizer_test__setup(finalized, old_memory);
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto last_collect = gks._d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(1_sz));
  AssertThat(last_collect[0] == old_memory, IsTrue());
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(1_sz));
  AssertThat(static_cast<bool>(finalized), IsTrue());
  // test bad parameters
  cgc1::cgc_register_finalizer(nullptr, [&finalized](void *) { finalized = true; });
  AssertThat(cgc1::cgc_start(nullptr) == nullptr, IsTrue());
  AssertThat(cgc1::cgc_start(&old_memory) == nullptr, IsTrue());
  cgc1::cgc_register_finalizer(&old_memory, [&finalized](void *) { finalized = true; });
}
static _NoInline_ void uncollectable_test__setup(size_t &old_memory)
{
  void *memory = cgc1::cgc_malloc(50);
  old_memory = cgc1::hide_pointer(memory);
  cgc1::cgc_set_uncollectable(memory, true);
  cgc1::secure_zero_pointer(memory);
}
static _NoInline_ void uncollectable_test__cleanup(size_t &old_memory)
{
  cgc1::cgc_set_uncollectable(cgc1::unhide_pointer(old_memory), false);
}
static void uncollectable_test()
{
  size_t old_memory;
  uncollectable_test__setup(old_memory);
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto last_collect = gks._d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(0_sz));
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(0_sz));
  uncollectable_test__cleanup(old_memory);
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  last_collect = gks._d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(1_sz)) AssertThat(last_collect[0] == old_memory, IsTrue());
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(1_sz));
  // test bad parameters
  cgc1::cgc_set_uncollectable(nullptr, true);
  cgc1::cgc_set_uncollectable(&old_memory, true);
}
static void linked_list_test()
{
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
  std::atomic<bool> keep_going{true};
  auto test_thread = [&keep_going]() {
    CGC1_INITIALIZE_THREAD();
    void **foo = reinterpret_cast<void **>(cgc1::cgc_malloc(100));
    {
      void **bar = foo;
      for (int i = 0; i < 3000; ++i) {
        {
          CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
          locations.push_back(cgc1::hide_pointer(bar));
        }
        cgc1::secure_zero(bar, 100);
        *bar = cgc1::cgc_malloc(100);
        bar = reinterpret_cast<void **>(*bar);
      }
      {
        CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
        locations.push_back(cgc1::hide_pointer(bar));
      }
    }
    while (keep_going) {
      ::std::stringstream ss;
      ss << foo << ::std::endl;
    }
    cgc1::cgc_unregister_thread();
  };
  ::std::thread t1(test_thread);
  ::std::this_thread::yield();
  for (int i = 0; i < 100; ++i) {
    cgc1::cgc_force_collect();
    gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
    auto freed_last = gks._d_freed_in_last_collection();
    assert(freed_last.empty());
    AssertThat(freed_last, HasLength(0));
#endif
    AssertThat(gks.num_freed_in_last_collection(), Equals(0_sz));
  }
  keep_going = false;
  t1.join();
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto last_collect = gks._d_freed_in_last_collection();
  ::std::sort(locations.begin(), locations.end());
  ::std::sort(last_collect.begin(), last_collect.end());
  AssertThat(last_collect.size(), Equals(locations.size()));
  bool all_found = ::std::equal(last_collect.begin(), last_collect.end(), locations.begin());
  assert(all_found);
  AssertThat(all_found, IsTrue());
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(locations.size()));
  locations.clear();
}
namespace race_condition_test_detail
{
  static ::std::vector<uintptr_t> llocations;
  static ::std::atomic<bool> keep_going{true};
  static ::std::atomic<size_t> finished_part1{0};

  static _NoInline_ void test_thread()
  {
    cgc1::clean_stack(0, 0, 0, 0, 0);
    CGC1_INITIALIZE_THREAD();
    void *foo = nullptr;
    {
      CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
      foo = cgc1::cgc_malloc(100);
      llocations.push_back(cgc1::hide_pointer(foo));
    }
    ++finished_part1;
    // syncronize with tests in main thread.
    while (keep_going) {
      //      ::std::this_thread::yield();
      ::std::this_thread::sleep_for(::std::chrono::milliseconds(1));
    }
    cgc1::secure_zero(&foo, sizeof(foo));
    {
      CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
      for (int i = 0; i < 1000; ++i) {
        llocations.push_back(cgc1::cgc_hidden_malloc(100));
      }
    }
    cgc1::cgc_unregister_thread();
    cgc1::clean_stack(0, 0, 0, 0, 0);
  }
  /**
   * \brief Try to create a race condition in the garbage collector.
 **/
  static _NoInline_ void race_condition_test()
  {
    const auto start_global_blocks = gks.gc_allocator().num_global_blocks();
    // these must be cleared each time to prevent race conditions.
    llocations.clear();
    keep_going = true;
    finished_part1 = 0;
    // get number of hardware threads
    const size_t num_threads = ::std::thread::hardware_concurrency() * 4;
    // const size_t num_threads = 2;
    // start threads
    ::std::vector<::std::thread> threads;
    for (size_t i = 0; i < num_threads; ++i)
      threads.emplace_back(test_thread);
    // try to force a race condition by interrupting threads.
    // this is obviously stochastic.
    while (finished_part1 != num_threads) {
      cgc1::cgc_force_collect();
      gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
      auto freed_last = gks._d_freed_in_last_collection();
      assert(freed_last.empty());
      AssertThat(freed_last, HasLength(0));
#endif
      AssertThat(gks.num_freed_in_last_collection(), Equals(0_sz));
      // prevent test from hammering gc before threads are setup.
      // sleep could cause deadlock with some osx platform functionality
      // therefore intentionally use it to try to break things.
      ::std::this_thread::sleep_for(::std::chrono::milliseconds(1));
    }
    // wait for threads to finish.
    keep_going = false;
    for (auto &&thread : threads)
      thread.join();
    // force collection.
    cgc1::cgc_force_collect();
    gks.wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
    auto last_collect = gks._d_freed_in_last_collection();
    // pointers might be in arbitrary order so sort them.
    ::std::sort(llocations.begin(), llocations.end());
    ::std::sort(last_collect.begin(), last_collect.end());
    // make sure all pointers have been collected.
    assert(last_collect.size() == 1001 * num_threads);
    AssertThat(last_collect, HasLength(1001 * num_threads));
    // check to make sure sizes are equal.
    AssertThat(last_collect.size(), Equals(llocations.size()));
    bool all_found = ::std::equal(last_collect.begin(), last_collect.end(), llocations.begin());
    assert(all_found);
    AssertThat(all_found, IsTrue());
    last_collect.clear();
#endif
    AssertThat(gks.num_freed_in_last_collection(), Equals(llocations.size()));
    // cleanup
    llocations.clear();
    // check to make sure empty blocks aren't being put in global blocks (ACTUAL BUG).
    gks.gc_allocator().collect();
    AssertThat(gks.gc_allocator().num_global_blocks(), Equals(start_global_blocks));
  }
}
static size_t expected_global_blocks(const size_t start, size_t taken, const size_t put_back)
{
  taken = ::std::min(start, taken);
  return start - taken + put_back;
}
static void return_to_global_test0()
{
  // get the global allocator
  auto &allocator = gks.gc_allocator();
  // get the number of global blocks starting.
  const auto start_num_global_blocks = allocator.num_global_blocks();
  // this thread will create an object (which will create a thread local block)
  // then it will exit, returning the block to global.
  auto test_thread = []() {
    cgc1::clean_stack(0, 0, 0, 0, 0);
    CGC1_INITIALIZE_THREAD();
    cgc1::cgc_malloc(100);
    cgc1::cgc_unregister_thread();
    cgc1::clean_stack(0, 0, 0, 0, 0);
  };
  ::std::thread t1(test_thread);
  t1.join();
  // wait for thread to terminate.
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
// make sure exactly one memory location was freed.
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto freed_last = gks._d_freed_in_last_collection();
  AssertThat(freed_last, HasLength(1));
#endif
  AssertThat(gks.num_freed_in_last_collection(), Equals(1_sz));
  // make sure that exactly one global block was added.
  auto num_global_blocks = allocator.num_global_blocks();
  AssertThat(num_global_blocks, Equals(expected_global_blocks(start_num_global_blocks, 1, 1)));
}
static void return_to_global_test1()
{
  // get the global allocator
  auto &allocator = gks.gc_allocator();
  // put block bounds here.
  ::std::atomic<uint8_t *> begin{nullptr};
  ::std::atomic<uint8_t *> end{nullptr};
  // thread local lambda.
  auto test_thread = [&allocator, &begin, &end]() {
    cgc1::clean_stack(0, 0, 0, 0, 0);
    CGC1_INITIALIZE_THREAD();
    // get thread local state.
    auto &tls = allocator.initialize_thread();
    const size_t size_to_alloc = 100;
    // get block set id.
    auto id = tls.find_block_set_id(size_to_alloc);
    auto &abs = tls.allocators()[id];
    // allocate some memory and immediately free it.
    cgc1::cgc_free(cgc1::cgc_malloc(size_to_alloc));
    // get the stats on the last block so that we can test to see if it is freed to global
    auto &lb = abs.last_block();
    begin = lb.begin();
    end = lb.end();
    cgc1::cgc_unregister_thread();
    cgc1::clean_stack(0, 0, 0, 0, 0);
  };
  ::std::thread t1(test_thread);
  t1.join();
  // wait for thread to terminate.
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
  // make sure nothing was freed (as it should have already been freed)
  auto freed_last = gks._d_freed_in_last_collection();
  AssertThat(freed_last, HasLength(0));
  // check that the memory was returned to global free list.
  auto pair = ::std::make_unique<::std::pair<uint8_t *, uint8_t *>>(begin, end);
  bool in_free = allocator.in_free_list(*pair);
  AssertThat(in_free, IsTrue());
  cgc1::secure_zero(&begin, sizeof(begin));
  cgc1::secure_zero(&end, sizeof(end));
}
static _NoInline_ void return_to_global_test2()
{
  cgc1::clean_stack(0, 0, 0, 0, 0);
  // get the global allocator
  auto &allocator = gks.gc_allocator();
  ::std::atomic<bool> ready_for_test{false};
  ::std::atomic<bool> test_done{false};
  ::std::atomic<uint8_t *> begin{nullptr};
  ::std::atomic<uint8_t *> end{nullptr};

  auto test_thread = [&allocator, &ready_for_test, &test_done, &begin, &end]() {
    cgc1::clean_stack(0, 0, 0, 0, 0);
    CGC1_INITIALIZE_THREAD();
    // get thread local state.
    auto &tls = allocator.initialize_thread();
    // get block set id.
    auto id = tls.find_block_set_id(100);
    auto &abs = tls.allocators()[id];
    // set destroy threshold to 0 so thread allocator always tries to return memory to global.
    tls.set_destroy_threshold(0);
    // set minimum local blocks to 1 so thread allocator tries to keep a block in reserve even if empty.
    tls.set_minimum_local_blocks(1);
    auto max_sz = abs.allocator_max_size();
    // create a bunch of memory allocations.
    ::std::vector<void *> ptrs;
    while (abs.size() != 2) {
      ptrs.push_back(cgc1::cgc_malloc(max_sz));
    }
    // get the stats on the last block so that we can test to see if it is freed to global.
    auto &lb1 = abs.last_block();
    begin = lb1.begin();
    end = lb1.end();
    tls.destroy(ptrs.back());
    ptrs.pop_back();
    ready_for_test = true;
    // wait for test to finish.
    while (!test_done)
      ::std::this_thread::yield();
    test_done = false;
    // reset stats on last block (it may not exist).
    begin = end = nullptr;
    // free the rest of the memory.
    // this should not actually release the block to global!
    for (auto &&ptr : ptrs) {
      tls.destroy(ptr);
    }
    ptrs.clear();
    // there should be a block left, but check before we try to access it.
    if (abs.size()) {
      // get the stats on the last block so that we can test to see if it is freed to global.
      auto &lb2 = abs.last_block();
      assert(lb2.valid());
      assert(lb2.empty());
      begin = lb2.begin();
      end = lb2.end();
    }
    ready_for_test = true;
    // wait for test to finish.
    while (!test_done)
      ::std::this_thread::yield();
    cgc1::cgc_unregister_thread();
    cgc1::clean_stack(0, 0, 0, 0, 0);
  };
  ::std::thread t1(test_thread);
  // wait for thread to setup.
  while (!ready_for_test)
    ::std::this_thread::yield();
  // Verify that we haven't created any global blocks.
  // Verify that the block that contained the freed allocation is now in the global free list.
  auto pair = ::std::make_unique<::std::pair<uint8_t *, uint8_t *>>(begin, end);
  bool in_free = allocator.in_free_list(*pair);
  assert(!in_free);
  AssertThat(!in_free, IsTrue());
  // reset readyness.
  ready_for_test = false;
  // signal to the thread that it can go ahead and finish.
  test_done = true;
  // wait for thread to setup next phase of testing.
  while (!ready_for_test)
    ::std::this_thread::yield();
  // the last block should not be in global free here.
  auto pair2 = ::std::make_unique<::std::pair<uint8_t *, uint8_t *>>(begin, end);
  auto in_free2 = allocator.in_free_list(*pair2);
  in_free = allocator.in_free_list(*pair);
  assert(in_free);
  AssertThat(in_free, IsTrue());
  AssertThat(in_free2, IsFalse());
  // done testing, resume thread.
  test_done = true;
  // wait for thread to terminate.
  t1.join();
  // now everything should be freed, so now the last block should be in global free.
  //  pair = ::std::make_unique<::std::pair<uint8_t *, uint8_t *>>(begin, end);
  in_free = allocator.in_free_list(*pair);
  AssertThat(in_free, IsTrue());
  AssertThat(in_free2, IsFalse());
  // force a collection to cleanup.
  cgc1::cgc_force_collect();
  gks.wait_for_finalization();
  // Verify that we haven't created any global blocks.
  cgc1::secure_zero(&begin, sizeof(begin));
  cgc1::secure_zero(&end, sizeof(end));
  cgc1::clean_stack(0, 0, 0, 0, 0);
}
/**
 * \brief Test various APIs.
**/
static void api_tests()
{
  // test heap size api call.
  AssertThat(cgc1::cgc_heap_size(), Is().GreaterThan(static_cast<size_t>(0)));
  // test heap free api call.
  AssertThat(cgc1::cgc_heap_free(), Is().GreaterThan(static_cast<size_t>(0)));
  // try disabling gc.
  cgc1::cgc_disable();
  AssertThat(cgc1::cgc_is_enabled(), Is().False());
  // try enabling gc.
  cgc1::cgc_enable();
  AssertThat(cgc1::cgc_is_enabled(), Is().True());
}

static void multiple_slab_test0()
{
  auto &fast_slab = gks._internal_fast_slab_allocator();
  constexpr const size_t packed_size = 4096 * 16;
  uint8_t *ret = reinterpret_cast<uint8_t *>(fast_slab.allocate_raw(packed_size));
  AssertThat(reinterpret_cast<uintptr_t>(ret) % 4096, Equals(32_sz));
  constexpr const size_t entry_size = 128ul;
  constexpr const size_t expected_entries = packed_size / entry_size - 2;
  using ps_type = ::cgc1::details::packed_object_state_t;
  static_assert(sizeof(ps_type) <= packed_size, "");
  AssertThat(reinterpret_cast<uintptr_t>(ret) % 32, Equals(0_sz));
  auto ps = new (ret) ps_type();
  (void)ps;

  constexpr const cgc1::details::packed_object_state_info_t state{1ul, entry_size, {0, 0}};
  ps->m_info = state;
  AssertThat(ps->size(), Equals(expected_entries));
  AssertThat(ps->total_size_bytes(), Equals(expected_entries * entry_size + 32 + 128));
  AssertThat(static_cast<size_t>(ps->end() - ret), Is().LessThanOrEqualTo(packed_size));
  fast_slab.deallocate_raw(ret);
}

static void multiple_slab_test0b()
{
  auto &fast_slab = gks._internal_fast_slab_allocator();
  constexpr const size_t packed_size = 4096 * 32;
  uint8_t *ret = reinterpret_cast<uint8_t *>(fast_slab.allocate_raw(packed_size));
  AssertThat(reinterpret_cast<uintptr_t>(ret) % 4096, Equals(32_sz));
  constexpr const size_t entry_size = 128ul;
  constexpr const size_t expected_entries = packed_size / entry_size - 3;
  using ps_type = ::cgc1::details::packed_object_state_t;
  static_assert(sizeof(ps_type) <= packed_size, "");
  AssertThat(reinterpret_cast<uintptr_t>(ret) % 32, Equals(0_sz));
  auto ps = new (ret) ps_type();
  (void)ps;

  constexpr const cgc1::details::packed_object_state_info_t state{2ul, entry_size, {0, 0}};
  ps->m_info = state;
  AssertThat(ps->size(), Equals(expected_entries));
  AssertThat(ps->total_size_bytes(), Equals(expected_entries * entry_size + 32 + 256));
  AssertThat(static_cast<size_t>(ps->end() - ret), Is().LessThanOrEqualTo(packed_size));
  fast_slab.deallocate_raw(ret);
}

static void multiple_slab_test1()
{
  using cgc1::details::c_packed_object_block_size;
  auto &fast_slab = gks._internal_fast_slab_allocator();
  fast_slab.align_next(c_packed_object_block_size);
  constexpr const size_t packed_size = 4096 * 8 - 32;
  uint8_t *ret = reinterpret_cast<uint8_t *>(fast_slab.allocate_raw(packed_size));
  AssertThat(reinterpret_cast<uintptr_t>(ret) % 4096, Equals(32_sz));
  constexpr const size_t entry_size = 64;
  constexpr const size_t expected_entries = packed_size / entry_size - 2;
  using ps_type = ::cgc1::details::packed_object_state_t;
  static_assert(sizeof(ps_type) <= packed_size, "");
  AssertThat(reinterpret_cast<uintptr_t>(ret) % 32, Equals(0_sz));
  auto ps = new (ret) ps_type();
  (void)ps;

  constexpr const cgc1::details::packed_object_state_info_t state{1ul, 64ul, {0, 0}};
  ps->m_info = state;
  AssertThat(ps->size(), Equals(expected_entries));
  AssertThat(static_cast<size_t>(ps->end() - ret), Is().LessThanOrEqualTo(packed_size));

  AssertThat(ret + packed_size >= ps->end(), IsTrue());
  AssertThat(ps->size(), Equals(expected_entries));
  ps->initialize();
  AssertThat(ps->any_free(), IsTrue());
  AssertThat(ps->none_free(), IsFalse());
  AssertThat(ps->first_free(), Equals(0_sz));
  ps->set_free(0, false);
  AssertThat(ps->first_free(), Equals(1_sz));
  for (size_t i = 0; i < 64; ++i) {
    ps->set_free(i, false);
  }
  AssertThat(ps->first_free(), Equals(64_sz));
  AssertThat(ps->is_free(53), IsFalse());
  AssertThat(ps->is_free(64), IsTrue());
  for (size_t i = 0; i < 255; ++i) {
    ps->set_free(i, false);
  }
  AssertThat(ps->first_free(), Equals(255_sz));
  ps->set_free(69, true);
  AssertThat(ps->first_free(), Equals(69_sz));

  ps->clear_mark_bits();
  AssertThat(ps->any_marked(), IsFalse());
  AssertThat(ps->none_marked(), IsTrue());
  ps->set_marked(0);
  AssertThat(ps->any_marked(), IsTrue());
  AssertThat(ps->none_marked(), IsFalse());
  for (size_t i = 0; i < 64; ++i) {
    if (i % 2)
      ps->set_marked(i);
  }
  AssertThat(ps->is_marked(0), IsTrue());
  AssertThat(ps->is_marked(31), IsTrue());
  AssertThat(ps->is_marked(33), IsTrue());
  for (size_t i = 200; i < 255; ++i) {
    if (i % 2)
      ps->set_marked(i);
  }
  for (size_t i = 1; i < 255; ++i) {
    if (i % 2 && (i < 64 || (i >= 200))) {
      AssertThat(ps->is_marked(i), IsTrue());
    } else {
      AssertThat(ps->is_marked(i), IsFalse());
    }
  }
  {
    ps->initialize();
    ps->clear_mark_bits();
    void *ptr = ps->allocate();
    AssertThat(ptr != nullptr, IsTrue());
    AssertThat(ps->is_free(0), IsFalse());
    AssertThat(ps->is_free(1), IsTrue());
    AssertThat(ps->deallocate(ptr), IsTrue());
    AssertThat(ps->is_free(0), IsTrue());
    AssertThat(ps->is_free(1), IsTrue());
  }
  ps->initialize();
  ps->clear_mark_bits();
  ::std::vector<void *> ptrs;
  bool keep_going = true;
  AssertThat(ps->num_blocks() * ps_type::bits_array_type::size(), Equals(8_sz));
  while (keep_going) {
    void *ptr = ps->allocate();
    if (ptr)
      ptrs.push_back(ptr);
    else {
      keep_going = false;
    }
  }
  AssertThat(ptrs.size(), Equals(expected_entries));
  for (size_t i = 0; i < ptrs.size(); ++i) {
    if (i % 3) {
      AssertThat(ps->deallocate(ptrs[i]), IsTrue());
    }
  }
  for (size_t i = 0; i < ptrs.size(); ++i) {
    if (i % 3) {
      AssertThat(ps->is_free(i), IsTrue());
    } else {
      AssertThat(ps->is_free(i), IsFalse());
    }
  }
  AssertThat(ps->is_free(3), IsFalse());
  AssertThat(ps->is_free(6), IsFalse());
  ps->set_marked(3);
  AssertThat(ps->is_marked(3), IsTrue());
  AssertThat(ps->is_marked(6), IsFalse());
  ps->free_unmarked();
  AssertThat(ps->is_free(3), IsFalse());
  AssertThat(ps->is_free(6), IsTrue());
  ps->set_free(3, true);
  AssertThat(ps->is_free(3), IsTrue());

  ::cgc1::details::packed_object_package_t package1;
  ::cgc1::details::packed_object_package_t package2;

  package1.insert(::std::move(package2));

  auto info0 = ::cgc1::details::packed_object_package_t::_get_info(0);
  AssertThat(info0.m_num_blocks, Equals(::cgc1::details::packed_object_package_t::cs_total_size / 512 / 32));
  AssertThat(info0.m_data_entry_sz, Equals(32_sz));
  auto info1 = ::cgc1::details::packed_object_package_t::_get_info(1);
  AssertThat(info1.m_num_blocks, Equals(::cgc1::details::packed_object_package_t::cs_total_size / 512 / 64));
  AssertThat(info1.m_data_entry_sz, Equals(64_sz));

  cgc1::details::packed_object_allocator_t poa(4096 * 4096, 4096 * 4096);

  cgc1::details::packed_object_thread_allocator_t ta(&poa);
  {
    void *v = ta.allocate(128);
    AssertThat(v != nullptr, IsTrue());
    ta.deallocate(v);
    uint8_t *v2 = reinterpret_cast<uint8_t *>(ta.allocate(128));
    uint8_t *v3 = reinterpret_cast<uint8_t *>(ta.allocate(128));
    AssertThat(reinterpret_cast<void *>(v), Equals(reinterpret_cast<void *>(v2)));
    AssertThat(reinterpret_cast<void *>(v3), Equals(reinterpret_cast<void *>(v2 + 128)));
  }

  AssertThat(cgc1::details::get_packed_object_size_id(31), Equals(0_sz));
  AssertThat(cgc1::details::get_packed_object_size_id(32), Equals(0_sz));
  AssertThat(cgc1::details::get_packed_object_size_id(33), Equals(1_sz));
  AssertThat(cgc1::details::get_packed_object_size_id(512), Equals(4_sz));
  AssertThat(cgc1::details::get_packed_object_size_id(513), Equals(::std::numeric_limits<size_t>::max()));
  //  cgc1::cgc_free(ret);
}
void gc_bandit_tests()
{
  describe("GC", []() {
    it("return_to_global_test0", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      return_to_global_test0();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });

    it("return_to_global_test1", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      return_to_global_test1();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    for (size_t i = 0; i < 1; ++i) {
      it("race condition", []() {
        cgc1::clean_stack(0, 0, 0, 0, 0);
        race_condition_test_detail::race_condition_test();
        cgc1::clean_stack(0, 0, 0, 0, 0);
      });
    }
    it("multiple_slab_test0", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      multiple_slab_test0();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("multiple_slab_test0b", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      multiple_slab_test0b();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });

    it("multiple_slab_test1", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      multiple_slab_test1();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("linked list test", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      linked_list_test();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("root", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      root_test();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("internal pointer", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      internal_pointer_test();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("finalizers", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      finalizer_test();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("atomic", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      atomic_test();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("uncollectable", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      uncollectable_test();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("api_tests", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      api_tests();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("return_to_global_test2", []() {
      cgc1::clean_stack(0, 0, 0, 0, 0);
      return_to_global_test2();
      cgc1::clean_stack(0, 0, 0, 0, 0);
    });
  });
}
