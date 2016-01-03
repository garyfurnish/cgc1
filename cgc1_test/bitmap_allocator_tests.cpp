#include "../cgc1/include/gc/gc.h"
#include "../cgc1/src/global_kernel_state.hpp"
#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include <mcppalloc/mcppalloc_bitmap_allocator/bitmap_allocator.hpp>
#include <mcppalloc/mcppalloc_utils/bandit.hpp>

using namespace bandit;
// alias
static auto &gks = ::cgc1::details::g_gks;
using namespace ::mcppalloc::literals;

/**
 * \brief Setup for root test.
 * This must be a separate funciton to make sure the compiler does not hide pointers somewhere.
 **/
static MCPPALLOC_NO_INLINE void packed_root_test__setup(void *&memory, size_t &old_memory, const size_t memory_sz)
{
  auto &poa = gks->_bitmap_allocator();
  auto &ta = poa.initialize_thread();
  memory = ta.allocate(memory_sz).m_ptr;
  AssertThat(::mcppalloc::is_zero(memory, memory_sz), IsTrue());
  mcppalloc::put_unique_seeded_random(memory, memory_sz);
  // hide a pointer away for comparison testing.
  old_memory = ::mcppalloc::hide_pointer(memory);
  cgc1::cgc_add_root(&memory);
  AssertThat(cgc1::cgc_size(memory), Equals(static_cast<size_t>(64)));
  AssertThat(cgc1::cgc_is_cgc(memory), IsTrue());
}

static void packed_root_test()
{
  const size_t memory_sz = 52;
  void *memory;
  size_t old_memory;
  // setup a root.
  packed_root_test__setup(memory, old_memory, memory_sz);
  auto state = mcppalloc::bitmap_allocator::details::get_state(memory);
  AssertThat(state->has_valid_magic_numbers(), IsTrue());
  // force collection
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  // verify that nothing was collected.
  state = mcppalloc::bitmap_allocator::details::get_state(memory);
  auto index = state->get_index(memory);
  AssertThat(state->has_valid_magic_numbers(), IsTrue());
  AssertThat(state->is_marked(index), IsTrue());
  AssertThat(state->is_free(index), IsFalse());
  // remove the root.
  cgc1::cgc_remove_root(&memory);
  // make sure that the we zero the memory so the pointer doesn't linger.
  ::mcppalloc::secure_zero_pointer(memory);
  const auto num_collections = cgc1::debug::num_gc_collections();
  // force collection.
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  index = state->get_index(::mcppalloc::unhide_pointer(old_memory));
  AssertThat(::mcppalloc::is_zero(::mcppalloc::unhide_pointer(old_memory), memory_sz), IsTrue());
  AssertThat(state->is_marked(index), IsFalse());
  AssertThat(state->is_free(index), IsTrue());
  // verify that we did perform a collection.
  AssertThat(cgc1::debug::num_gc_collections(), Equals(num_collections + 1));
}

static void packed_linked_list_test()
{
  ::std::vector<uintptr_t> locations;
  ::mcppalloc::mutex_t debug_mutex;
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  std::atomic<bool> keep_going{true};
  // this SHOULD NOT NEED TO BE HERE>
  void **tmp;
  cgc1::cgc_add_root(reinterpret_cast<void **>(&tmp));
  auto test_thread = [&keep_going, &debug_mutex, &locations, &tmp]() {
    CGC1_INITIALIZE_THREAD();
    //    void** tmp;
    auto &poa = gks->_bitmap_allocator();
    auto &ta = poa.initialize_thread();
    tmp = reinterpret_cast<void **>(ta.allocate(100).m_ptr);
    {
      void **bar = tmp;
      for (int i = 0; i < 0; ++i) {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(debug_mutex);
        locations.push_back(::mcppalloc::hide_pointer(bar));
        ::mcppalloc::secure_zero(bar, 100);
        *bar = ta.allocate(100).m_ptr;
        bar = reinterpret_cast<void **>(*bar);
      }
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(debug_mutex);
        // locations.push_back(::mcppalloc::hide_pointer(bar));
        locations.push_back(::mcppalloc::hide_pointer(tmp));
      }
    }
    while (keep_going) {
      ::std::stringstream ss;
      ss << tmp << ::std::endl;
    }
    cgc1::cgc_unregister_thread();
  };
  ::std::thread t1(test_thread);
  ::std::this_thread::yield();
  ::std::this_thread::sleep_for(::std::chrono::seconds(1));
  for (int i = 0; i < 100; ++i) {
    cgc1::cgc_force_collect();
    gks->wait_for_finalization();
    MCPPALLOC_CONCURRENCY_LOCK_GUARD(debug_mutex);
    for (auto &&loc : locations) {
      if (!cgc1::debug::_cgc_hidden_packed_marked(loc)) {
        ::std::cerr << "pointer not marked " << ::mcppalloc::unhide_pointer(loc) << ::std::endl;
        ::std::abort();
      }
      if (mcppalloc_unlikely(cgc1::debug::_cgc_hidden_packed_free(loc))) {
        ::std::cerr << "097af1d8-8bfd-433e-b46d-88c6b0dc7dce";
        ::std::abort();
      }
    }
  }
  ::cgc1::clean_stack(0, 0, 0, 0, 0);
  keep_going = false;
  t1.join();
  cgc1::cgc_remove_root(reinterpret_cast<void **>(&tmp));
  ::mcppalloc::secure_zero(&tmp, sizeof(tmp));
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  for (auto &&loc : locations) {
    auto state = mcppalloc::bitmap_allocator::details::get_state(::mcppalloc::unhide_pointer(loc));
    auto index = state->get_index(::mcppalloc::unhide_pointer(loc));
    AssertThat(state->has_valid_magic_numbers(), IsTrue());
    AssertThat(state->is_marked(index), IsFalse());
    AssertThat(state->is_free(index), IsTrue());
  }
  locations.clear();
}
static MCPPALLOC_NO_INLINE bool is_unique_seeded_random(void *v, size_t sz)
{
  return ::mcppalloc::is_unique_seeded_random(v, sz);
}
static void packed_allocator_test()
{
  ::std::vector<uintptr_t> locations;
  ::mcppalloc::mutex_t debug_mutex;
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  std::atomic<bool> keep_going{true};
  void *tmp{nullptr};
  cgc1::cgc_root root(tmp);
  const size_t allocation_size = sizeof(size_t) * 10;
  ::std::atomic<bool> is_finalized{false};
  auto test_thread = [&is_finalized, &keep_going, &debug_mutex, &locations, &tmp, allocation_size]() {
    CGC1_INITIALIZE_THREAD();

    tmp = cgc1::cgc_malloc(allocation_size);
    ::mcppalloc::bitmap_allocator::details::get_state(tmp)->verify_magic();
    if (!mcppalloc::is_zero(tmp, allocation_size)) {
      ::std::cerr << "c3c1b941-f503-4730-b1bf-fd72861348e1\n";
      assert(0);
      ::std::abort();
    }
    mcppalloc::put_unique_seeded_random(tmp, allocation_size);
    auto finalizer = [&is_finalized](void *) { is_finalized = true; };
    cgc1::cgc_register_finalizer(tmp, finalizer);
    assert(mcppalloc::is_unique_seeded_random(tmp, allocation_size));
    if (!mcppalloc::is_unique_seeded_random(tmp, allocation_size)) {
      ::std::cerr << "eb5d2cda-f75a-4e04-8e0c-eb302b536e01\n";
      assert(0);
      ::std::abort();
    }
    void *tmp2 = cgc1::cgc_malloc(100);
    (void)tmp2;
    if (!mcppalloc::is_unique_seeded_random(tmp, allocation_size)) {
      ::std::cerr << __FILE__ << " " << __LINE__ << " " << tmp << " " << tmp2 << "\n";
      ::std::cerr << __FILE__ << " " << __LINE__ << " " << mcppalloc::is_unique_seeded_random_failure_loc(tmp, allocation_size)
                  << ::std::endl;
      assert(0);
      ::std::abort();
    }
    locations.push_back(::mcppalloc::hide_pointer(tmp));

    ::mcppalloc::bitmap_allocator::details::get_state(::mcppalloc::unhide_pointer(locations.back()))->verify_magic();
    void *tmp3 = cgc1::cgc_malloc(500);
    (void)tmp3;
    while (keep_going) {
      ::std::stringstream ss;
      ss << tmp << ::std::endl;
    }
    cgc1::cgc_unregister_thread();
  };
  ::std::thread t1(test_thread);
  ::std::this_thread::yield();
  ::std::this_thread::sleep_for(::std::chrono::seconds(1));
  for (int i = 0; i < 100; ++i) {
    cgc1::cgc_force_collect();
    gks->wait_for_finalization();
    MCPPALLOC_CONCURRENCY_LOCK_GUARD(debug_mutex);
    for (auto &&loc : locations) {
      if (!cgc1::debug::_cgc_hidden_packed_marked(loc)) {
        ::std::cerr << "pointer not marked " << ::mcppalloc::unhide_pointer(loc) << ::std::endl;
        ::std::abort();
      }
      if (mcppalloc_unlikely(cgc1::debug::_cgc_hidden_packed_free(loc))) {
        ::std::cerr << "8c6bda38-3d6b-4632-9b5a-1f20dde0b222\n";
        ::std::abort();
      }
    }
  }
  ::cgc1::clean_stack(0, 0, 0, 0, 0);
  keep_going = false;
  t1.join();
  if (!is_unique_seeded_random(tmp, allocation_size)) {
    ::std::cerr << "1b646f7b-c852-4069-a91f-605b40f19da1\n";
    ::std::abort();
  }
  ::cgc1::clean_stack(0, 0, 0, 0, 0);
  root.clear();
  ::mcppalloc::secure_zero_pointer(tmp);
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  for (auto &&loc : locations) {
    auto state = mcppalloc::bitmap_allocator::details::get_state(::mcppalloc::unhide_pointer(loc));
    auto index = state->get_index(::mcppalloc::unhide_pointer(loc));
    AssertThat(state->has_valid_magic_numbers(), IsTrue());
    AssertThat(state->is_marked(index), IsFalse());
    AssertThat(state->is_free(index), IsTrue());
  }
  //  AssertThat(is_finalized.load(), IsTrue());
  locations.clear();
}

void gc_bitmap_tests()
{
  describe("GC", []() {
    it("packed_root_test", []() { packed_root_test(); });
    it("packed_linked_list_test", []() { packed_linked_list_test(); });
    it("packed_allocator_test", []() { packed_allocator_test(); });
  });
}
