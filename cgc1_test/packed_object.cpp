#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include "bandit.hpp"
#include "../cgc1/src/global_kernel_state.hpp"
#include "../cgc1/src/packed_object_state.hpp"
#include "../cgc1/src/packed_object_allocator.hpp"
using namespace bandit;

static auto &gks = ::cgc1::details::g_gks;
using namespace ::cgc1::literals;

static void packed_object_state_test0()
{
  using cgc1::details::c_packed_object_block_size;
  auto &fast_slab = gks->_packed_object_allocator()._slab();
  fast_slab.align_next(c_packed_object_block_size);
  constexpr const size_t packed_size = 4096 * 8 - 32;
  uint8_t *ret = reinterpret_cast<uint8_t *>(fast_slab.allocate_raw(packed_size));
  AssertThat(reinterpret_cast<uintptr_t>(ret) % 4096, Equals(32_sz));
  constexpr const size_t entry_size = 64;
  constexpr const size_t expected_entries = packed_size / entry_size - 3;
  using ps_type = ::cgc1::details::packed_object_state_t;
  static_assert(sizeof(ps_type) <= packed_size, "");
  AssertThat(reinterpret_cast<uintptr_t>(ret) % 32, Equals(0_sz));
  auto ps = new (ret) ps_type();
  (void)ps;

  constexpr const cgc1::details::packed_object_state_info_t state{1ul, 64ul, expected_entries, 0, {0, 0, 0, 0}};
  ps->m_info = state;
  ps->_compute_size();
  AssertThat(ps->size(), Equals(expected_entries));
  AssertThat(static_cast<size_t>(ps->end() - ret), IsLessThanOrEqualTo(packed_size));
  auto max_end = ret + packed_size;
  AssertThat(max_end, IsGreaterThanOrEqualTo(ps->end()));
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
  fast_slab.deallocate_raw(ret);
}

static void multiple_slab_test0()
{
  auto &fast_slab = gks->_packed_object_allocator()._slab();
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

  constexpr const cgc1::details::packed_object_state_info_t state{
      1ul, entry_size, expected_entries, sizeof(cgc1::details::packed_object_state_info_t) + 128, {0, 0, 0, 0}};
  ps->m_info = state;
  AssertThat(ps->size(), Equals(expected_entries));
  AssertThat(ps->total_size_bytes(),
             Equals(expected_entries * entry_size + sizeof(cgc1::details::packed_object_state_info_t) + 128));
  AssertThat(static_cast<size_t>(ps->end() - ret), Is().LessThanOrEqualTo(packed_size));
  fast_slab.deallocate_raw(ret);
}

static void multiple_slab_test0b()
{
  auto &fast_slab = gks->_packed_object_allocator()._slab();
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

  constexpr const cgc1::details::packed_object_state_info_t state{
      2ul, entry_size, expected_entries, sizeof(cgc1::details::packed_object_state_info_t) + 256, {0, 0, 0, 0}};
  ps->m_info = state;
  AssertThat(ps->size(), Equals(expected_entries));
  AssertThat(ps->total_size_bytes(),
             Equals(expected_entries * entry_size + sizeof(cgc1::details::packed_object_state_info_t) + 256));
  AssertThat(static_cast<size_t>(ps->end() - ret), Is().LessThanOrEqualTo(packed_size));
  fast_slab.deallocate_raw(ret);
}

static void multiple_slab_test1()
{

  ::cgc1::details::packed_object_package_t package1;
  ::cgc1::details::packed_object_package_t package2;

  package1.insert(::std::move(package2));

  auto info0 = ::cgc1::details::packed_object_package_t::_get_info(0);
  AssertThat(info0.m_num_blocks, Equals(::cgc1::details::packed_object_package_t::cs_total_size / 512 / 32));
  AssertThat(info0.m_data_entry_sz, Equals(32_sz));
  auto info1 = ::cgc1::details::packed_object_package_t::_get_info(1);
  AssertThat(info1.m_num_blocks, Equals(::cgc1::details::packed_object_package_t::cs_total_size / 512 / 64));
  AssertThat(info1.m_data_entry_sz, Equals(64_sz));

  cgc1::details::packed_object_allocator_t &poa = cgc1::details::g_gks->_packed_object_allocator();

  cgc1::details::packed_object_thread_allocator_t &ta = poa.initialize_thread();
  {
    void *v = ta.allocate(128);
    AssertThat(v != nullptr, IsTrue());
    AssertThat(ta.deallocate(v), IsTrue());
    uint8_t *v2 = reinterpret_cast<uint8_t *>(ta.allocate(128));
    uint8_t *v3 = reinterpret_cast<uint8_t *>(ta.allocate(128));
    AssertThat(reinterpret_cast<void *>(v), Equals(reinterpret_cast<void *>(v2)));
    AssertThat(reinterpret_cast<void *>(v3), Equals(reinterpret_cast<void *>(v2 + 128)));
    ta.deallocate(v2);
    uint8_t *v4 = reinterpret_cast<uint8_t *>(ta.allocate(128));
    AssertThat(reinterpret_cast<void *>(v2), Equals(reinterpret_cast<void *>(v4)));
    ta.deallocate(v3);
    ta.deallocate(v4);
  }
  poa.destroy_thread();
  // make sure destroying twice does not crash.
  poa.destroy_thread();
  AssertThat(poa.num_free_blocks(), Equals(1_sz));
  for (size_t i = 0; i < cgc1::details::packed_object_package_t::cs_num_vectors; ++i)
    AssertThat(poa.num_globals(i), Equals(0_sz));
  // ok, now take and check that destroying with a valid block puts it in globals not freed.
  auto &ta2 = poa.initialize_thread();
  {
    void *v = ta2.allocate(128);
    AssertThat(v != nullptr, IsTrue());
  }
  poa.destroy_thread();
  AssertThat(poa.num_free_blocks(), Equals(0_sz));
  AssertThat(poa.num_globals(cgc1::details::get_packed_object_size_id(128)), Equals(1_sz));
  AssertThat(cgc1::details::get_packed_object_size_id(31), Equals(0_sz));
  AssertThat(cgc1::details::get_packed_object_size_id(32), Equals(0_sz));
  AssertThat(cgc1::details::get_packed_object_size_id(33), Equals(1_sz));
  AssertThat(cgc1::details::get_packed_object_size_id(512), Equals(4_sz));
  AssertThat(cgc1::details::get_packed_object_size_id(513), Equals(::std::numeric_limits<size_t>::max()));
  //  cgc1::cgc_free(ret);
}
/**
 * \brief Setup for root test.
 * This must be a separate funciton to make sure the compiler does not hide pointers somewhere.
 **/
static _NoInline_ void packed_root_test__setup(void *&memory, size_t &old_memory)
{
  cgc1::details::packed_object_allocator_t &poa = cgc1::details::g_gks->_packed_object_allocator();
  cgc1::details::packed_object_thread_allocator_t &ta = poa.initialize_thread();
  memory = ta.allocate(50);
  // hide a pointer away for comparison testing.
  old_memory = cgc1::hide_pointer(memory);
  cgc1::cgc_add_root(&memory);
  AssertThat(cgc1::cgc_size(memory), Equals(static_cast<size_t>(64)));
  AssertThat(cgc1::cgc_is_cgc(memory), IsTrue());
}

static void packed_root_test()
{
  void *memory;
  size_t old_memory;
  // setup a root.
  packed_root_test__setup(memory, old_memory);
  auto state = cgc1::details::get_state(memory);
  AssertThat(state->has_valid_magic_numbers(), IsTrue());
  // force collection
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  // verify that nothing was collected.
  state = cgc1::details::get_state(memory);
  auto index = state->get_index(memory);
  AssertThat(state->has_valid_magic_numbers(), IsTrue());
  AssertThat(state->is_marked(index), IsTrue());
  AssertThat(state->is_free(index), IsFalse());
  // remove the root.
  cgc1::cgc_remove_root(&memory);
  // make sure that the we zero the memory so the pointer doesn't linger.
  cgc1::secure_zero_pointer(memory);
  auto num_collections = cgc1::debug::num_gc_collections();
  // force collection.
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  index = state->get_index(cgc1::unhide_pointer(old_memory));
  AssertThat(state->is_marked(index), IsFalse());
  AssertThat(state->is_free(index), IsTrue());
  // verify that we did perform a collection.
  AssertThat(cgc1::debug::num_gc_collections(), Equals(num_collections + 1));
}

static void packed_linked_list_test()
{
  ::std::vector<uintptr_t> locations;
  cgc1::mutex_t debug_mutex;
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  std::atomic<bool> keep_going{true};
  // this SHOULD NOT NEED TO BE HERE>
  void **foo;
  cgc1::cgc_add_root(reinterpret_cast<void **>(&foo));
  auto test_thread = [&keep_going, &debug_mutex, &locations, &foo]() {
    CGC1_INITIALIZE_THREAD();
    //    void** foo;
    cgc1::details::packed_object_allocator_t &poa = cgc1::details::g_gks->_packed_object_allocator();
    cgc1::details::packed_object_thread_allocator_t &ta = poa.initialize_thread();
    foo = reinterpret_cast<void **>(ta.allocate(100));
    {
      void **bar = foo;
      for (int i = 0; i < 0; ++i) {
        CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
        locations.push_back(cgc1::hide_pointer(bar));
        cgc1::secure_zero(bar, 100);
        *bar = ta.allocate(100);
        bar = reinterpret_cast<void **>(*bar);
      }
      {
        CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
        // locations.push_back(cgc1::hide_pointer(bar));
        locations.push_back(cgc1::hide_pointer(foo));
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
  ::std::this_thread::sleep_for(::std::chrono::seconds(1));
  for (int i = 0; i < 100; ++i) {
    //    CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
    cgc1::cgc_force_collect();
    gks->wait_for_finalization();
    CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
    for (auto &&loc : locations) {
      if (!cgc1::debug::_cgc_hidden_packed_marked(loc)) {
        ::std::cerr << "pointer not marked " << cgc1::unhide_pointer(loc) << ::std::endl;
        abort();
      }
      if (cgc1::debug::_cgc_hidden_packed_free(loc))
        abort();
      //      AssertThat(cgc1::debug::_cgc_hidden_packed_marked(loc), IsTrue());
      //      AssertThat(cgc1::debug::_cgc_hidden_packed_free(loc), IsFalse());
    }
  }
  cgc1::clean_stack(0, 0, 0, 0, 0);
  keep_going = false;
  t1.join();
  cgc1::cgc_remove_root(reinterpret_cast<void **>(&foo));
  cgc1::secure_zero(&foo, sizeof(foo));
  cgc1::cgc_force_collect();
  gks->wait_for_finalization();
  for (auto &&loc : locations) {
    auto state = cgc1::details::get_state(cgc1::unhide_pointer(loc));
    auto index = state->get_index(cgc1::unhide_pointer(loc));
    AssertThat(state->has_valid_magic_numbers(), IsTrue());
    AssertThat(state->is_marked(index), IsFalse());
    AssertThat(state->is_free(index), IsTrue());
  }
  locations.clear();
}

void packed_object_tests()
{
  describe("packed_object_tests", []() {
    it("packed_object_state_test0", []() { packed_object_state_test0(); });
    it("multiple_slab_test0", []() { multiple_slab_test0(); });
    it("multiple_slab_test0b", []() { multiple_slab_test0b(); });

    it("multiple_slab_test1", []() { multiple_slab_test1(); });
    it("packed_root_test", []() { packed_root_test(); });
    (void)packed_linked_list_test;
    it("packed_linked_list_test", []() { packed_linked_list_test(); });
  });
}
