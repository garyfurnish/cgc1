#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include "bandit.hpp"
#include <cgc1/posix_slab.hpp>
#include <cgc1/posix.hpp>
#include <cgc1/aligned_allocator.hpp>
#include "../cgc1/src/slab_allocator.hpp"
#include "../cgc1/src/allocator_block.hpp"
#include "../cgc1/src/allocator.hpp"
using namespace ::bandit;
using namespace ::cgc1::literals;
void allocator_tests()
{
  describe("allocator", []() {
    it("test1", []() {
      {
        ::std::unique_ptr<cgc1::details::allocator_t<>> allocator(new cgc1::details::allocator_t<>());
        AssertThat(allocator->initialize(100000, 100000000), IsTrue());
        cgc1::details::allocator_t<>::this_thread_allocator_t ta(*allocator);
        auto &allocator_set = ta.allocator_by_size(55);
        AssertThat(allocator_set.allocator_min_size(), Is().LessThan(55_sz));
        AssertThat(allocator_set.allocator_max_size(), Is().GreaterThan(55_sz));
        AssertThat(ta.set_allocator_multiple(20, 50), IsFalse());
        void *alloc1 = ta.allocate(100);
        AssertThat(alloc1 != nullptr, IsTrue());
        void *alloc2 = ta.allocate(100);
        AssertThat(alloc2 != nullptr, IsTrue());
        void *alloc3 = ta.allocate(100);
        AssertThat(alloc3 != nullptr, IsTrue());
        AssertThat(ta.destroy(alloc1), IsTrue());
        AssertThat(ta.allocate(100), Equals(alloc1));
      }
    });
    it("test2", []() {
      ::std::unique_ptr<cgc1::details::allocator_t<>> allocator(new cgc1::details::allocator_t<>());
      AssertThat(allocator->initialize(200000, 100000000), IsTrue());
      // test worst free list.
      auto memory1 = allocator->get_memory(10000);
      auto memory2 = allocator->get_memory(20000);
      auto memory3 = allocator->get_memory(10000);
      AssertThat(memory1.first != nullptr, IsTrue());
      allocator->release_memory(memory1);
      allocator->release_memory(memory2);
      AssertThat(allocator->_d_free_list(), HasLength(2));
      auto memory2_new = allocator->get_memory(10000);
      AssertThat(memory2_new.first, Equals(memory2.first));
      AssertThat(memory2_new.second, Equals(memory2.first + 10000));
      auto memory4 = allocator->get_memory(10000);
      AssertThat(memory4.first, Equals(memory2_new.second));
      AssertThat(memory4.second, Equals(memory2.second));
      AssertThat(allocator->get_memory(10000), Equals(memory1));
      AssertThat(allocator->_d_free_list(), HasLength(0));
      allocator->release_memory(memory3);
      allocator->release_memory(memory2_new);
      allocator->release_memory(memory1);
      AssertThat(allocator->current_end() != allocator->begin(), IsTrue());
      allocator->collapse();
      AssertThat(allocator->_d_free_list(), HasLength(1));
      allocator->release_memory(memory4);
      AssertThat(allocator->_d_free_list(), HasLength(1));
      allocator->collapse();
      AssertThat(allocator->_d_free_list(), HasLength(0));
      AssertThat(static_cast<void *>(allocator->current_end()), Equals(static_cast<void *>(allocator->begin())));
    });
    it("test3", []() {
      ::std::unique_ptr<cgc1::details::allocator_t<>> allocator(new cgc1::details::allocator_t<>());
      AssertThat(allocator->initialize(100000, 100000000), IsTrue());
      allocator->initialize_thread();
      allocator->destroy_thread();
      auto &ta = allocator->initialize_thread();
      void *alloc1 = ta.allocate(100);
      AssertThat(alloc1 != nullptr, IsTrue());
      void *alloc2 = ta.allocate(100);
      AssertThat(alloc2 != nullptr, IsTrue());
      void *alloc3 = ta.allocate(100);
      AssertThat(alloc3 != nullptr, IsTrue());
      AssertThat(ta.destroy(alloc1), IsTrue());
      AssertThat(ta.allocate(100), Equals(alloc1));
    });
    it("test4", []() {
      for (size_t j = 0; j < 30; ++j) {
        ::std::array<cgc1::rebind_vector_t<size_t, cgc1::cgc_internal_allocator_t<size_t>>, 5> vecs;
        for (size_t i = 0; i < 10000; ++i) {
          for (auto &vec : vecs)
            vec.push_back(i);
        }
        for (size_t i = 0; i < 10000; ++i) {
          for (auto &vec : vecs)
            AssertThat(vec[i], Equals(i));
        }
      }
    });
    it("test_abs_remove_block", []() {
      // make an allocator.
      auto allocator = ::std::make_unique<::cgc1::details::allocator_t<>>();
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
        ptrs.emplace_back(tls.allocate(size_to_alloc));
      ptrs.pop_back();
      AssertThat(abs.m_blocks.size(), Equals(2_sz));
      // get rid of one of the blocks by freeing everything in it.
      for (auto &&ptr : ptrs)
        tls.destroy(ptr);
      AssertThat(abs.m_blocks.size(), Equals(1_sz));
      {
        // the one block that is left should have the same address in registration as in the block set.
        CGC1_CONCURRENCY_LOCK_GUARD(allocator->_mutex());
        assert(allocator->_u_blocks().size() == 1);
        AssertThat(allocator->_u_blocks(), HasLength(1));
        bool block_correct = &abs.m_blocks.front() == allocator->_u_blocks()[0].m_block;
        assert(block_correct);
        AssertThat(block_correct, IsTrue());
      }
      allocator->destroy_thread();
    });
    it("thread_allocator_do_maintenance", []() {
      ::std::unique_ptr<cgc1::details::allocator_t<>> allocator(new cgc1::details::allocator_t<>());
      AssertThat(allocator->initialize(100000, 100000000), IsTrue());
      cgc1::details::allocator_t<>::this_thread_allocator_t ta(*allocator);
      ta._do_maintenance();
    });
    it("test_global_block_recycling", []() {
      ::std::unique_ptr<cgc1::details::allocator_t<>> allocator(new cgc1::details::allocator_t<>());
      AssertThat(allocator->initialize(100000, 100000000), IsTrue());
      auto &ta = allocator->initialize_thread();
      void *alloc1 = ta.allocate(100);
      AssertThat(alloc1 != nullptr, IsTrue());
      allocator->destroy_thread();
      auto &ta2 = allocator->initialize_thread();
      void *alloc2 = ta2.allocate(100);
      (void)alloc2;
    });
  });
}
