#include <mcpputil/mcpputil/declarations.hpp>
//This Must be first.
#include <mcppalloc/mcppalloc_sparse/mcppalloc_sparse.hpp>
#include <mcpputil/mcpputil/aligned_allocator.hpp>
#include <mcpputil/mcpputil/bandit.hpp>
#include <mcpputil/mcpputil/literals.hpp>
#include <mcpputil/mcpputil/memory_range.hpp>
using namespace ::bandit;
using namespace ::mcpputil::literals;
void allocator_tests()
{
  describe("allocator", []() {
    using policy = ::mcppalloc::default_allocator_policy_t<::mcpputil::default_aligned_allocator_t>;
    using allocator_type = ::mcppalloc::sparse::allocator_t<policy>;
    using ta_type = allocator_type::thread_allocator_type;
    it("test1", []() {
      {
        ::std::unique_ptr<allocator_type> allocator(new allocator_type());
        AssertThat(allocator->initialize(100000, 100000000), IsTrue());
        ta_type ta(*allocator);
        auto &allocator_set = ta.allocator_by_size(55);
        AssertThat(allocator_set.allocator_min_size(), Is().LessThan(55_sz));
        AssertThat(allocator_set.allocator_max_size(), Is().GreaterThan(55_sz));
        AssertThat(ta.set_allocator_multiple(20, 50), IsFalse());
        void *alloc1 = ta.allocate(100).m_ptr;
        AssertThat(alloc1 != nullptr, IsTrue());
        void *alloc2 = ta.allocate(100).m_ptr;
        AssertThat(alloc2 != nullptr, IsTrue());
        void *alloc3 = ta.allocate(100).m_ptr;
        AssertThat(alloc3 != nullptr, IsTrue());
        AssertThat(ta.destroy(alloc1), IsTrue());
        AssertThat(ta.allocate(100).m_ptr, Equals(alloc1));
      }
    });
    it("test2", []() {
      ::std::unique_ptr<allocator_type> allocator(new allocator_type());
      AssertThat(allocator->initialize(200000, 100000000), IsTrue());
      // test worst free list.
      ::mcpputil::system_memory_range_t memory1 = allocator->get_memory(10000, false);
      auto memory2 = allocator->get_memory(20000, false);
      auto memory3 = allocator->get_memory(10000, false);
      AssertThat(::mcpputil::size(memory1), Equals(10000_sz));
      AssertThat(::mcpputil::size(memory2), Equals(20000_sz));
      AssertThat(::mcpputil::size(memory3), Equals(10000_sz));
      AssertThat(memory1.begin() != nullptr, IsTrue());
      allocator->release_memory(memory1);
      allocator->release_memory(memory2);
      AssertThat(allocator->_d_free_list(), HasLength(2));
      auto memory2_new = allocator->get_memory(10000, false);
      AssertThat(::mcpputil::size(memory2_new), Equals(10000_sz));
      AssertThat(memory2_new.begin(), Equals(memory1.begin()));
      AssertThat(reinterpret_cast<int *>(memory2_new.end()), Equals(reinterpret_cast<int *>(memory1.begin() + 10000)));
      auto memory4 = allocator->get_memory(10000, false);
      AssertThat(memory4.begin(), Equals(memory2_new.end()));
      AssertThat(memory4.end(), Equals(memory2.begin() + 10000));
      auto memory5 = allocator->get_memory(10000, false);
      AssertThat(memory5, Equals(mcpputil::system_memory_range_t(memory2.begin() + 10000, memory2.end())));
      AssertThat(allocator->_d_free_list(), HasLength(0));
      allocator->release_memory(memory3);
      allocator->release_memory(memory2_new);
      AssertThat(allocator->current_end() != allocator->begin(), IsTrue());
      auto vec = allocator->_d_free_list();

      allocator->collapse();
      vec = allocator->_d_free_list();
      AssertThat(allocator->_d_free_list(), HasLength(1));
      allocator->release_memory(memory4);
      vec = allocator->_d_free_list();
      AssertThat(allocator->_d_free_list(), HasLength(2));
      allocator->release_memory(memory5);
      allocator->collapse();
      vec = allocator->_d_free_list();
      AssertThat(allocator->_d_free_list(), HasLength(0));
      AssertThat(static_cast<void *>(allocator->current_end()), Equals(static_cast<void *>(allocator->begin())));
      //*/
    });
    it("test3", []() {
      ::std::unique_ptr<allocator_type> allocator = ::std::make_unique<allocator_type>();
      AssertThat(allocator->initialize(100000, 100000000), IsTrue());
      allocator->initialize_thread();
      allocator->destroy_thread();
      auto &ta = allocator->initialize_thread();
      void *alloc1 = ta.allocate(100).m_ptr;
      AssertThat(alloc1 != nullptr, IsTrue());
      void *alloc2 = ta.allocate(100).m_ptr;
      AssertThat(alloc2 != nullptr, IsTrue());
      void *alloc3 = ta.allocate(100).m_ptr;
      AssertThat(alloc3 != nullptr, IsTrue());
      AssertThat(ta.destroy(alloc1), IsTrue());
      AssertThat(ta.allocate(100).m_ptr, Equals(alloc1));
    });
    it("test4", []() {
      /*for (size_t j = 0; j < 30; ++j) {
      ::std::array<mcppalloc::rebind_vector_t<size_t, mcppalloc::cgc_internal_allocator_t<size_t>>, 5> vecs;
      for (size_t i = 0; i < 10000; ++i) {
        for (auto &vec : vecs)
          vec.push_back(i);
      }
      for (size_t i = 0; i < 10000; ++i) {
        for (auto &vec : vecs)
          AssertThat(vec[i], Equals(i));
      }
      }*/
    });
    it("test_abs_remove_block", []() {
      // make an allocator.
      auto allocator = ::std::make_unique<allocator_type>();
      AssertThat(allocator->initialize(100000, 100000000), IsTrue());
      // get thread local state.
      auto &tls = allocator->initialize_thread();
      // ok, for this as soon as possible we want to get rid of the extra blocks from abs.
      tls.set_destroy_threshold(0);
      tls.set_minimum_local_blocks(0);
      // so we need to get the abs.
      const size_t size_to_alloc = 100;
      // get block set id.
      auto id = tls.find_block_set_id(size_to_alloc);
      auto &abs = tls.allocators()[id];
      // list to store allocate pointers in so we can destroy them.
      ::std::vector<void *> ptrs;
      while (abs.m_blocks.size() != 2)
        ptrs.emplace_back(tls.allocate(size_to_alloc).m_ptr);
      ptrs.pop_back();
      AssertThat(abs.m_blocks.size(), Equals(2_sz));
      // get rid of one of the blocks by freeing everything in it.
      for (auto &&ptr : ptrs)
        tls.destroy(ptr);
      AssertThat(abs.m_blocks.size(), Equals(1_sz));
      {
        // the one block that is left should have the same address in registration as in the block set.
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(allocator->_mutex());
        assert(allocator->_u_blocks().size() == 1);
        AssertThat(allocator->_u_blocks(), HasLength(1));
        bool block_correct = &abs.m_blocks.front() == allocator->_u_blocks()[0].m_block;
        assert(block_correct);
        AssertThat(block_correct, IsTrue());
      }
      allocator->destroy_thread();
    });
    it("thread_allocator_do_maintenance", []() {
      auto allocator = ::std::make_unique<allocator_type>();
      AssertThat(allocator->initialize(100000, 100000000), IsTrue());
      ta_type ta(*allocator);
      ta._do_maintenance();
    });
    it("test_global_block_recycling", []() {
      auto allocator = ::std::make_unique<allocator_type>();
      AssertThat(allocator->initialize(100000, 100000000), IsTrue());
      auto &ta = allocator->initialize_thread();
      void *alloc1 = ta.allocate(100).m_ptr;
      AssertThat(alloc1 != nullptr, IsTrue());
      allocator->destroy_thread();
      auto &ta2 = allocator->initialize_thread();
      void *alloc2 = ta2.allocate(100).m_ptr;
      (void)alloc2;
    });
  });
}
