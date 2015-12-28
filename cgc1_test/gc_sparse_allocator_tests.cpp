#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include <mcppalloc/mcppalloc_utils/bandit.hpp>
#include <thread>
#include <chrono>
#include <string.h>
#include <signal.h>
#include <mcppalloc/mcppalloc_sparse/allocator.hpp>
#include "../cgc1/src/internal_allocator.hpp"
#include "../cgc1/src/global_kernel_state.hpp"
#include "../cgc1/src/internal_stream.hpp"
#include "../cgc1/include/gc/gc.h"
static ::std::vector<size_t> locations;
static ::mcppalloc::spinlock_t debug_mutex;
using namespace bandit;
// alias
static auto &gks = ::cgc1::details::g_gks;
using namespace ::mcppalloc::literals;

/**
 * \brief Setup for root test.
 * This must be a separate funciton to make sure the compiler does not hide pointers somewhere.
 **/
static MCPPALLOC_NO_INLINE void root_test__setup(void *&memory, size_t &old_memory)
{
  auto &ta = gks->gc_allocator().initialize_thread();
  memory = ta.allocate(50).m_ptr;
  // hide a pointer away for comparison testing.
  old_memory = ::mcppalloc::hide_pointer(memory);
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
  gks->wait_for_finalization();
  // verify that nothing was collected.
  auto last_collect = gks->_d_freed_in_last_collection();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  AssertThat(last_collect, HasLength(0));
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(0_sz));
  // remove the root.
  cgc1::cgc_remove_root(&memory);
  // make sure that the we zero the memory so the pointer doesn't linger.
  ::mcppalloc::secure_zero_pointer(memory);
  auto num_collections = cgc1::debug::num_gc_collections();
  // force collection.
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  last_collect = gks->_d_freed_in_last_collection();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  // now we should collect.
  AssertThat(last_collect.size(), Equals(1_sz));
  // verify it collected the correct address.
  AssertThat(last_collect[0] == old_memory, IsTrue());
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(1_sz));
  // verify that we did perform a collection.
  AssertThat(cgc1::debug::num_gc_collections(), Equals(num_collections + 1));
}
/**
 * \brief Setup for internal pointer test.
 **/
static MCPPALLOC_NO_INLINE void internal_pointer_test__setup(void *&memory, size_t &old_memory)
{
  auto &ta = gks->gc_allocator().initialize_thread();
  memory = ta.allocate(50).m_ptr;
  uint8_t *&umemory = ::mcppalloc::unsafe_reference_cast<uint8_t *>(memory);
  old_memory = ::mcppalloc::hide_pointer(memory);
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
  gks->wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto last_collect = gks->_d_freed_in_last_collection();
  // it should stick around because it has a root.
  AssertThat(last_collect, HasLength(0));
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(0_sz));
  // remove the root.
  cgc1::cgc_remove_root(&memory);
  // make sure that we zero the memory so the pointer doesn't linger.
  ::mcppalloc::secure_zero_pointer(memory);
  // force collection.
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  last_collect = gks->_d_freed_in_last_collection();
  // now we should collect.
  AssertThat(last_collect, HasLength(1));
  // verify it collected the correct address.
  AssertThat(last_collect[0] == old_memory, IsTrue());
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(1_sz));
}
/**
 * \brief Setup for atomic object test.
 **/
static MCPPALLOC_NO_INLINE void atomic_test__setup(void *&memory, size_t &old_memory)
{
  auto &ta = gks->gc_allocator().initialize_thread();
  memory = ta.allocate(50).m_ptr;
  cgc1::cgc_add_root(&memory);
  void *memory2 = ta.allocate(50).m_ptr;
  *reinterpret_cast<void **>(memory) = memory2;
  old_memory = ::mcppalloc::hide_pointer(memory2);
  ::mcppalloc::secure_zero_pointer(memory2);
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
  gks->wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto last_collect = gks->_d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(0_sz));
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(0_sz));
  cgc1::cgc_set_atomic(memory, true);
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  last_collect = gks->_d_freed_in_last_collection();

  AssertThat(last_collect, HasLength(1));
  AssertThat(last_collect[0] == old_memory, IsTrue());
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(1_sz));
  cgc1::cgc_remove_root(&memory);
  ::mcppalloc::secure_zero_pointer(memory);
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  AssertThat(last_collect, HasLength(1));
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(1_sz));
  // test bad parameters
  cgc1::cgc_set_atomic(nullptr, true);
  cgc1::cgc_set_atomic(&old_memory, true);
}
static MCPPALLOC_NO_INLINE void finalizer_test__setup(std::atomic<bool> &finalized, size_t &old_memory)
{
  auto &ta = gks->gc_allocator().initialize_thread();
  void *memory = ta.allocate(50).m_ptr;
  old_memory = ::mcppalloc::hide_pointer(memory);
  finalized = false;
  cgc1::cgc_register_finalizer(memory, [&finalized](void *) { finalized = true; });
  ::mcppalloc::secure_zero_pointer(memory);
}
static void finalizer_test()
{
  size_t old_memory;
  std::atomic<bool> finalized;
  finalizer_test__setup(finalized, old_memory);
  ::cgc1::clean_stack(0, 0, 0, 0, 0);
  cgc1::cgc_force_collect(true);
  cgc1::cgc_wait_finalization();

#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto last_collect = gks->_d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(1_sz));
  AssertThat(last_collect[0] == old_memory, IsTrue());
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(1_sz));
  AssertThat(static_cast<bool>(finalized), IsTrue());
  // test bad parameters
  cgc1::cgc_register_finalizer(nullptr, [&finalized](void *) { finalized = true; }, false, false);
  AssertThat(cgc1::cgc_start(nullptr) == nullptr, IsTrue());
  AssertThat(cgc1::cgc_start(&old_memory) == nullptr, IsTrue());
  cgc1::cgc_register_finalizer(&old_memory, [&finalized](void *) { finalized = true; }, false, false);
}
static MCPPALLOC_NO_INLINE void finalizer_test2__setup(::std::atomic<bool> &finalized, size_t &old_memory)
{
  auto &ta = gks->gc_allocator().initialize_thread();
  void *memory = ta.allocate(50).m_ptr;
  old_memory = ::mcppalloc::hide_pointer(memory);
  finalized = false;
  cgc1::cgc_register_finalizer(memory, [&finalized](void *) { finalized = true; }, true);
  ::mcppalloc::secure_zero_pointer(memory);
}
static void finalizer_test2()
{
  size_t old_memory;
  std::atomic<bool> finalized{false};
  finalizer_test2__setup(finalized, old_memory);
  ::cgc1::clean_stack(0, 0, 0, 0, 0);
  cgc1::cgc_force_collect(false);
  cgc1::cgc_wait_finalization(false);
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto last_collect = gks->_d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(1_sz));
  AssertThat(last_collect[0] == old_memory, IsTrue());
#endif
  AssertThat(finalized.load(), IsTrue());
  AssertThat(gks->num_freed_in_last_collection(), Equals(1_sz));
  // test bad parameters
  cgc1::cgc_register_finalizer(nullptr, [&finalized](void *) { finalized = true; }, false, false);
  AssertThat(cgc1::cgc_start(nullptr) == nullptr, IsTrue());
  AssertThat(cgc1::cgc_start(&old_memory) == nullptr, IsTrue());
  cgc1::cgc_register_finalizer(&old_memory, [&finalized](void *) { finalized = true; }, false, false);
}

static MCPPALLOC_NO_INLINE void uncollectable_test__setup(size_t &old_memory)
{
  auto &ta = gks->gc_allocator().initialize_thread();
  void *memory = ta.allocate(50).m_ptr;
  old_memory = ::mcppalloc::hide_pointer(memory);
  cgc1::cgc_set_uncollectable(memory, true);
  ::mcppalloc::secure_zero_pointer(memory);
}
static MCPPALLOC_NO_INLINE void uncollectable_test__cleanup(size_t &old_memory)
{
  cgc1::cgc_set_uncollectable(::mcppalloc::unhide_pointer(old_memory), false);
}
static void uncollectable_test()
{
  size_t old_memory;
  uncollectable_test__setup(old_memory);
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto last_collect = gks->_d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(0_sz));
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(0_sz));
  uncollectable_test__cleanup(old_memory);
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  last_collect = gks->_d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(1_sz)) AssertThat(last_collect[0] == old_memory, IsTrue());
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(1_sz));
  // test bad parameters
  cgc1::cgc_set_uncollectable(nullptr, true);
  cgc1::cgc_set_uncollectable(&old_memory, true);
}

static void linked_list_test_setup()
{
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  std::atomic<bool> keep_going{true};
  auto test_thread = [&keep_going]() {
    CGC1_INITIALIZE_THREAD();
    auto &ta = gks->gc_allocator().initialize_thread();
    const size_t allocation_size = 50_sz;
    void **foo = reinterpret_cast<void **>(ta.allocate(allocation_size).m_ptr);
    {
      void **bar = foo;
      for (int i = 0; i < 3000; ++i) {
        {
          MCPPALLOC_CONCURRENCY_LOCK_GUARD(debug_mutex);
          locations.push_back(::mcppalloc::hide_pointer(bar));
        }
        ::mcppalloc::secure_zero(bar, allocation_size);
        *bar = ta.allocate(allocation_size).m_ptr;
        bar = reinterpret_cast<void **>(*bar);
      }
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(debug_mutex);
        locations.push_back(::mcppalloc::hide_pointer(bar));
      }
      ::mcppalloc::secure_zero_pointer(bar);
    }
    while (keep_going) {
      ::std::stringstream ss;
      ss << foo << ::std::endl;
    }
    ::mcppalloc::secure_zero_pointer(foo);
    ::cgc1::cgc_unregister_thread();
    ::cgc1::clean_stack(0, 0, 0, 0, 0);
  };
  ::cgc1::clean_stack(0, 0, 0, 0, 0);
  auto t1 = ::std::make_unique<::std::thread>(test_thread);
  ::std::this_thread::yield();
  for (int i = 0; i < 100; ++i) {
    cgc1::cgc_force_collect();
    gks->wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
    auto freed_last = gks->_d_freed_in_last_collection();
    assert(freed_last.empty());
    AssertThat(freed_last, HasLength(0));
#endif
    AssertThat(gks->num_freed_in_last_collection(), Equals(0_sz));
  }
  keep_going = false;
  t1->join();
  ::mcppalloc::secure_zero(&test_thread, sizeof(test_thread));
}
static void linked_list_final()
{
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto last_collect = gks->_d_freed_in_last_collection();
  ::std::sort(locations.begin(), locations.end());
  ::std::sort(last_collect.begin(), last_collect.end());
  AssertThat(last_collect.size(), Equals(locations.size()));
  bool all_found = ::std::equal(last_collect.begin(), last_collect.end(), locations.begin());
  assert(all_found);
  AssertThat(all_found, IsTrue());
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(locations.size()));
  locations.clear();
}
static void linked_list_test()
{
  linked_list_test_setup();
  ::cgc1::clean_stack(0, 0, 0, 0, 0);
  linked_list_final();
}
namespace race_condition_test_detail
{
  static ::std::vector<uintptr_t> llocations;
  static ::std::atomic<bool> keep_going{true};
  static ::std::atomic<size_t> finished_part1{0};

  static MCPPALLOC_NO_INLINE void test_thread()
  {
    ::cgc1::clean_stack(0, 0, 0, 0, 0);
    CGC1_INITIALIZE_THREAD();
    void *foo = nullptr;
    auto &ta = gks->gc_allocator().initialize_thread();
    {
      MCPPALLOC_CONCURRENCY_LOCK_GUARD(debug_mutex);
      foo = ta.allocate(100).m_ptr;
      llocations.push_back(::mcppalloc::hide_pointer(foo));
    }
    ++finished_part1;
    // syncronize with tests in main thread.
    while (keep_going) {
      //      ::std::this_thread::yield();
      ::std::this_thread::sleep_for(::std::chrono::milliseconds(1));
    }
    ::mcppalloc::secure_zero(&foo, sizeof(foo));
    {
      MCPPALLOC_CONCURRENCY_LOCK_GUARD(debug_mutex);
      for (int i = 0; i < 1000; ++i) {
        llocations.push_back(::mcppalloc::hide_pointer(ta.allocate(100).m_ptr));
      }
    }
    cgc1::cgc_unregister_thread();
    ::cgc1::clean_stack(0, 0, 0, 0, 0);
  }
  /**
   * \brief Try to create a race condition in the garbage collector.
 **/
  static MCPPALLOC_NO_INLINE void race_condition_test()
  {
    const auto start_global_blocks = gks->gc_allocator().num_global_blocks();
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
      gks->wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
      auto freed_last = gks->_d_freed_in_last_collection();
      assert(freed_last.empty());
      AssertThat(freed_last, HasLength(0));
#endif
      AssertThat(gks->num_freed_in_last_collection(), Equals(0_sz));
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
    gks->wait_for_finalization();
#ifdef CGC1_DEBUG_VERBOSE_TRACK
    auto last_collect = gks->_d_freed_in_last_collection();
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
    AssertThat(gks->num_freed_in_last_collection(), Equals(llocations.size()));
    // cleanup
    llocations.clear();
    // check to make sure empty blocks aren't being put in global blocks (ACTUAL BUG).
    gks->gc_allocator().collect();
    AssertThat(gks->gc_allocator().num_global_blocks(), Equals(start_global_blocks));
    ::cgc1::clean_stack(0, 0, 0, 0, 0);
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
  auto &allocator = gks->gc_allocator();
  // get the number of global blocks starting.
  const auto start_num_global_blocks = allocator.num_global_blocks();
  // this thread will create an object (which will create a thread local block)
  // then it will exit, returning the block to global.
  auto test_thread = []() {
    ::cgc1::clean_stack(0, 0, 0, 0, 0);
    CGC1_INITIALIZE_THREAD();
    auto &ta = gks->gc_allocator().initialize_thread();
    ta.allocate(100);
    cgc1::cgc_unregister_thread();
    ::cgc1::clean_stack(0, 0, 0, 0, 0);
  };
  ::std::thread t1(test_thread);
  t1.join();
  // wait for thread to terminate.
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
// make sure exactly one memory location was freed.
#ifdef CGC1_DEBUG_VERBOSE_TRACK
  auto freed_last = gks->_d_freed_in_last_collection();
  AssertThat(freed_last, HasLength(1));
#endif
  AssertThat(gks->num_freed_in_last_collection(), Equals(1_sz));
  // make sure that exactly one global block was added.
  auto num_global_blocks = allocator.num_global_blocks();
  AssertThat(num_global_blocks, Equals(expected_global_blocks(start_num_global_blocks, 1, 1)));
}
static void return_to_global_test1()
{
  // get the global allocator
  auto &allocator = gks->gc_allocator();
  // put block bounds here.
  ::std::atomic<uint8_t *> begin{nullptr};
  ::std::atomic<uint8_t *> end{nullptr};
  // thread local lambda.
  auto test_thread = [&allocator, &begin, &end]() {
    ::cgc1::clean_stack(0, 0, 0, 0, 0);
    CGC1_INITIALIZE_THREAD();
    // get thread local state.
    auto &tls = allocator.initialize_thread();
    const size_t size_to_alloc = 100;
    // get block set id.
    auto id = tls.find_block_set_id(size_to_alloc);
    auto &abs = tls.allocators()[id];
    // allocate some memory and immediately free it.
    auto &ta = gks->gc_allocator().initialize_thread();
    cgc1::cgc_free(ta.allocate(size_to_alloc).m_ptr);
    // get the stats on the last block so that we can test to see if it is freed to global
    auto &lb = *abs.last_block();
    begin = lb.begin();
    end = lb.end();
    cgc1::cgc_unregister_thread();
    ::cgc1::clean_stack(0, 0, 0, 0, 0);
  };
  ::std::thread t1(test_thread);
  t1.join();
  // wait for thread to terminate.
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  // make sure nothing was freed (as it should have already been freed)
  auto freed_last = gks->_d_freed_in_last_collection();
  AssertThat(freed_last, HasLength(0));
  // check that the memory was returned to global free list.
  auto pair = ::std::make_unique<::std::pair<uint8_t *, uint8_t *>>(begin, end);
  bool in_free = allocator.in_free_list(*pair);
  AssertThat(in_free, IsTrue());
  ::mcppalloc::secure_zero(&begin, sizeof(begin));
  ::mcppalloc::secure_zero(&end, sizeof(end));
}
static MCPPALLOC_NO_INLINE void return_to_global_test2()
{
  ::cgc1::clean_stack(0, 0, 0, 0, 0);
  // get the global allocator
  auto &allocator = gks->gc_allocator();
  ::std::atomic<bool> ready_for_test{false};
  ::std::atomic<bool> test_done{false};
  ::std::atomic<uint8_t *> begin{nullptr};
  ::std::atomic<uint8_t *> end{nullptr};

  auto test_thread = [&allocator, &ready_for_test, &test_done, &begin, &end]() {
    ::cgc1::clean_stack(0, 0, 0, 0, 0);
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
    auto &ta = gks->gc_allocator().initialize_thread();
    while (abs.size() != 2) {
      ptrs.push_back(ta.allocate(max_sz).m_ptr);
    }
    // get the stats on the last block so that we can test to see if it is freed to global.
    auto &lb1 = *abs.last_block();
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
      auto &lb2 = *abs.last_block();
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
    ::cgc1::clean_stack(0, 0, 0, 0, 0);
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
  gks->wait_for_finalization();
  // Verify that we haven't created any global blocks.
  ::mcppalloc::secure_zero(&begin, sizeof(begin));
  ::mcppalloc::secure_zero(&end, sizeof(end));
  ::cgc1::clean_stack(0, 0, 0, 0, 0);
}
/**
 * \brief Test various APIs.
**/
static void api_tests()
{
  GC_get_version();
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

void gc_bandit_tests()
{
  describe("GC_sparse", []() {
    it("return_to_global_test0", []() {
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
      return_to_global_test0();
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
    });

    it("return_to_global_test1", []() {
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
      return_to_global_test1();
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    for (size_t i = 0; i < 1; ++i) {
      it("race condition", []() {
        ::cgc1::clean_stack(0, 0, 0, 0, 0);
        race_condition_test_detail::race_condition_test();
        ::cgc1::clean_stack(0, 0, 0, 0, 0);
      });
    }
    it("linked list test", []() {
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
      linked_list_test();
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("root", []() {
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
      root_test();
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("internal pointer", []() {
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
      internal_pointer_test();
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("finalizers", []() {
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
      finalizer_test();
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("finalizer2", []() {
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
      finalizer_test2();
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("atomic", []() {
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
      atomic_test();
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("uncollectable", []() {
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
      uncollectable_test();
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("api_tests", []() {
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
      api_tests();
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
    });
    it("return_to_global_test2", []() {
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
      return_to_global_test2();
      ::cgc1::clean_stack(0, 0, 0, 0, 0);
    });
  });
}
