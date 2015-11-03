
/**
 * \brief Setup for root test.
 * This must be a separate funciton to make sure the compiler does not hide pointers somewhere.
 **/
static CGC1_NO_INLINE void packed_root_test__setup(void *&memory, size_t &old_memory)
{
  using allocator_type =
      ::mcppalloc::bitmap_allocator::bitmap_allocator_t<::mcppalloc::default_allocator_policy_t<::std::allocator<void>>>;
  allocator_type bitmap_allocator(200000, 200000);
  auto &poa = bitmap_allocator->_bitmap_allocator();
  auto &ta = poa.initialize_thread();
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
    auto &poa = mcppalloc::bitmap_allocator::details::g_gks->_bitmap_allocator();
    auto &ta = poa.initialize_thread();
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
    auto state = mcppalloc::bitmap_allocator::details::get_state(cgc1::unhide_pointer(loc));
    auto index = state->get_index(cgc1::unhide_pointer(loc));
    AssertThat(state->has_valid_magic_numbers(), IsTrue());
    AssertThat(state->is_marked(index), IsFalse());
    AssertThat(state->is_free(index), IsTrue());
  }
  locations.clear();
}

it("multiple_slab_test1", []() { multiple_slab_test1(); });
it("packed_root_test", []() { packed_root_test(); });
(void) packed_linked_list_test;
it("packed_linked_list_test", []() { packed_linked_list_test(); });
