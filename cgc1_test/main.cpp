#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include <bandit/bandit.h>
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
#ifdef _WIN32
#pragma optimize("", off)
#endif
// Statics
static ::std::vector<void *> locations;
static ::std::mutex debug_mutex;
using namespace bandit;
go_bandit([]() {
  describe("Pages", []() {
    it("test1", []() {
      cgc1::slab_t page(cgc1::slab_t::page_size() * 50, cgc1::slab_t::find_hole(cgc1::slab_t::page_size() * 10000));
#ifndef __APPLE__
      AssertThat(page.expand(cgc1::slab_t::page_size() * 5000), IsTrue());
      AssertThat(page.size() >= cgc1::slab_t::page_size() * 5000, IsTrue());
#endif
      page.destroy();
    });
  });
  describe("Slab Allocator", []() {
    it("test1", []() {
      using cgc1::details::slab_allocator_object_t;
      cgc1::details::slab_allocator_t slab(500000, 5000000);
      void *alloc1 = slab.allocate_raw(100);
      AssertThat(alloc1 == slab.begin() + 16, IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->next() == &*slab._u_object_current_end(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->next()->next_valid(), IsFalse());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->next()->not_available(), IsFalse());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->not_available(), IsTrue());
      void *alloc2 = slab.allocate_raw(100);
      AssertThat(alloc2 == slab.begin() + 144, IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc2)->next() == &*slab._u_object_current_end(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc2)->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc2)->next()->next_valid(), IsFalse());
      AssertThat(slab_allocator_object_t::from_object_start(alloc2)->not_available(), IsTrue());
      void *alloc3 = slab.allocate_raw(100);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      void *alloc4 = slab.allocate_raw(100);
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
      slab.deallocate_raw(alloc4);
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next() == &*slab._u_object_current_end(), IsTrue());
      alloc4 = slab.allocate_raw(100);
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
      slab.deallocate_raw(alloc4);
      slab.deallocate_raw(alloc3);
      AssertThat(slab_allocator_object_t::from_object_start(alloc2)->next() == &*slab._u_object_current_end(), IsTrue());
      alloc3 = slab.allocate_raw(100);
      alloc4 = slab.allocate_raw(100);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
      slab.deallocate_raw(alloc3);
      slab.deallocate_raw(alloc4);
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next() == &*slab._u_object_current_end(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next()->next_valid(), IsFalse());
      alloc3 = slab.allocate_raw(100);
      alloc4 = slab.allocate_raw(100);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc4)->next() == &*slab._u_object_current_end(), IsTrue());
      void *alloc5 = slab.allocate_raw(100);
      AssertThat(alloc5 == slab.begin() + 528, IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc5)->next() == &*slab._u_object_current_end(), IsTrue());
      slab.deallocate_raw(alloc4);
      slab.deallocate_raw(alloc3);
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next_valid(), IsTrue());
      alloc3 = slab.allocate_raw(100);
      alloc4 = slab.allocate_raw(100);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc4)->next_valid(), IsTrue());
      // Now test coalescing.
      slab.deallocate_raw(alloc3);
      slab.deallocate_raw(alloc4);
      alloc3 = slab.allocate_raw(239);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      // Now test splitting.
      slab.deallocate_raw(alloc3);
      alloc3 = slab.allocate_raw(100);
      alloc4 = slab.allocate_raw(100);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
    });
    it("test2,", []() {
      cgc1::rebind_vector_t<int, cgc1::cgc_internal_slab_allocator_t<int>> vec;
      cgc1::rebind_vector_t<int, cgc1::cgc_internal_slab_allocator_t<int>> vec2;
      cgc1::rebind_vector_t<int, cgc1::cgc_internal_slab_allocator_t<int>> vec3;
      cgc1::rebind_vector_t<int, cgc1::cgc_internal_slab_allocator_t<int>> vec4;
      for (int i = 0; i < 1000; ++i) {
        vec.push_back(i);
        vec2.push_back(i);
        vec3.push_back(i);
        vec4.push_back(i);
      }
      for (int i = 0; i < 1000; ++i) {
        AssertThat(vec[i], Equals(i));
        AssertThat(vec2[i], Equals(i));
        AssertThat(vec3[i], Equals(i));
        AssertThat(vec4[i], Equals(i));
      }
    });
  });
  describe("Allocator", []() {
    describe("Block", []() {
      void *memory = malloc(100);
      using cgc1::details::object_state_t;
      cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t> block(memory, 96, 16);
      void *alloc1, *alloc2, *alloc3, *alloc4;
      it("alloc1", [&]() {
        alloc1 = block.allocate(15);
        AssertThat(alloc1, !Equals((void *)nullptr));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->not_available(), IsTrue());
      });
      it("alloc2", [&]() {
        alloc2 = block.allocate(15);
        AssertThat(alloc2, !Equals((void *)nullptr));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->not_available(), IsTrue());
      });
      it("alloc3", [&]() {
        alloc3 = block.allocate(15);
        AssertThat(alloc3, !Equals((void *)nullptr));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc3)->next_valid(), IsFalse());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc3)->not_available(), IsTrue());
        auto alloc3_state = object_state_t::from_object_start(alloc3);
        AssertThat(alloc3_state->next() == reinterpret_cast<object_state_t *>(block.end()), IsTrue());
      });
      it("alloc4", [&]() {
        alloc4 = block.allocate(15);
        AssertThat(alloc4, Equals((void *)nullptr));
      });
      it("re-alloc 3", [&]() {
        auto old_alloc3 = alloc3;
        block.destroy(alloc3);
        alloc3 = block.allocate(15);
        AssertThat(alloc3, Equals(old_alloc3));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc3)->next_valid(), IsFalse());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc3)->not_available(), IsTrue());
      });
      it("re-alloc 2", [&]() {
        auto old_alloc2 = alloc2;
        block.destroy(alloc2);
        alloc2 = block.allocate(15);
        AssertThat(alloc2, Equals(old_alloc2));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->not_available(), IsTrue());
      });
      it("re-alloc 2&3", [&]() {
        auto old_alloc3 = alloc3;
        auto old_alloc2 = alloc2;
        block.destroy(alloc2);
        block.destroy(alloc3);
        AssertThat(block.m_free_list, HasLength(1));
        alloc2 = block.allocate(15);
        AssertThat(alloc2, Equals(old_alloc2));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->not_available(), IsTrue());
        alloc3 = block.allocate(15);
        AssertThat(alloc3, Equals(old_alloc3));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc3)->next_valid(), IsFalse());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc3)->not_available(), IsTrue());
      });
      it("re-alloc 2&3 #2", [&]() {
        auto old_alloc3 = alloc3;
        auto old_alloc2 = alloc2;
        auto alloc3_state = object_state_t::from_object_start(alloc3);
        AssertThat(alloc3_state->next() == reinterpret_cast<object_state_t *>(block.end()), IsTrue());
        block.destroy(alloc3);
        AssertThat(alloc3_state->next_valid(), !IsTrue());
        AssertThat(alloc3_state->next() == reinterpret_cast<object_state_t *>(block.end()), IsTrue());
        AssertThat(alloc3_state->in_use(), !IsTrue());
        block.destroy(alloc2);
        AssertThat(block.m_free_list, HasLength(0));
        alloc2 = block.allocate(15);
        AssertThat(alloc2, Equals(old_alloc2));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->not_available(), IsTrue());
        alloc3 = block.allocate(15);
        AssertThat(alloc3, Equals(old_alloc3));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc3)->next_valid(), IsFalse());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc3)->not_available(), IsTrue());
      });
      it("realloc 1&2", [&]() {
        auto old_alloc2 = alloc2;
        auto old_alloc1 = alloc1;
        block.destroy(alloc1);
        block.destroy(alloc2);
        AssertThat(block.m_free_list, HasLength(2));
        alloc2 = block.allocate(15);
        AssertThat(alloc2, Equals(old_alloc2));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->not_available(), IsTrue());
        alloc1 = block.allocate(15);
        AssertThat(alloc1, Equals(old_alloc1));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->not_available(), IsTrue());
      });
      it("realloc 1&2 #2", [&]() {
        auto old_alloc2 = alloc2;
        auto old_alloc1 = alloc1;
        block.destroy(alloc2);
        block.destroy(alloc1);
        AssertThat(block.m_free_list, HasLength(1));
        alloc1 = block.allocate(15);
        AssertThat(alloc1, Equals(old_alloc1));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->not_available(), IsTrue());
        AssertThat(block.m_free_list, HasLength(1));
        alloc2 = block.allocate(15);
        AssertThat(alloc2, Equals(old_alloc2));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->not_available(), IsTrue());
        AssertThat(block.m_free_list, HasLength(0));
      });
      it("realloc 1&2 #3", [&]() {
        auto old_alloc1 = alloc1;
        auto old_alloc2 = alloc2;
        block.destroy(alloc2);
        block.destroy(alloc1);
        AssertThat(block.m_free_list, HasLength(1));
        alloc1 = block.allocate(48);
        AssertThat(alloc1, Equals(old_alloc1));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->not_available(), IsTrue());
        AssertThat(block.m_free_list, HasLength(0));
        block.destroy(alloc1);
        AssertThat(block.m_free_list, HasLength(1));
        alloc1 = block.allocate(15);
        AssertThat(alloc1, Equals(old_alloc1));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->not_available(), IsTrue());
        AssertThat(block.m_free_list, HasLength(1));
        alloc2 = block.allocate(15);
        AssertThat(alloc2, Equals(old_alloc2));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->not_available(), IsTrue());
        AssertThat(block.m_free_list, HasLength(0));
      });
      it("Fail realloc 1&2 #2", [&]() {
        auto old_alloc2 = alloc2;
        auto old_alloc1 = alloc1;
        block.destroy(alloc2);
        block.destroy(alloc1);
        AssertThat(block.m_free_list, HasLength(1));
        AssertThat(block.allocate(49), Equals((void *)nullptr));
        alloc1 = block.allocate(15);
        AssertThat(alloc1, Equals(old_alloc1));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->not_available(), IsTrue());
        AssertThat(block.m_free_list, HasLength(1));
        alloc2 = block.allocate(15);
        AssertThat(alloc2, Equals(old_alloc2));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->not_available(), IsTrue());
        AssertThat(block.m_free_list, HasLength(0));
      });
      it("fail massive alloc", []() {
        void *memory2 = malloc(1000);
        cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t> block2(memory2, 96, 16);
        AssertThat(block2.allocate(985), Equals((void *)nullptr));
        free(memory2);
      });
      it("empty", []() {
        void *memory2 = malloc(1000);
        cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t> block2(memory2, 992, 16);
        AssertThat(block2.empty(), IsTrue());
        auto m = block2.allocate(500);
        AssertThat(block2.empty(), IsFalse());
        block2.destroy(m);
        AssertThat(block2.empty(), IsTrue());
        free(memory2);
      });
      it("collect", [&]() {
        auto old_alloc3 = alloc3;
        auto old_alloc2 = alloc2;
        auto old_alloc1 = alloc1;
        block.destroy(alloc1);
        block.destroy(alloc2);
        AssertThat(block.m_free_list, HasLength(2));
        block.collect();
        AssertThat(block.m_free_list, HasLength(1));
        block.destroy(alloc3);
        AssertThat(block.m_free_list, HasLength(1));
        block.collect();
        AssertThat(block.m_free_list, HasLength(0));
        alloc1 = block.allocate(15);
        AssertThat(alloc1, Equals(old_alloc1));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc1)->not_available(), IsTrue());
        alloc2 = block.allocate(15);
        AssertThat(alloc2, Equals(old_alloc2));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->next_valid(), IsTrue());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc2)->not_available(), IsTrue());
        alloc3 = block.allocate(15);
        AssertThat(alloc3, Equals(old_alloc3));
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc3)->next_valid(), IsFalse());
        AssertThat(cgc1::details::object_state_t::from_object_start(alloc3)->not_available(), IsTrue());
      });
      it("find", [&]() {
        AssertThat(block.find_address(alloc2) == cgc1::details::object_state_t::from_object_start(alloc2), IsTrue());
        AssertThat(block.find_address(reinterpret_cast<uint8_t*>(alloc2) + 1) == cgc1::details::object_state_t::from_object_start(alloc2), IsTrue());
        AssertThat(block.find_address(reinterpret_cast<uint8_t*>(alloc2) + 50) == nullptr, IsTrue());
      });
      free(memory);
    });
    describe("allocator_block_set", []() {
      void *memory1 = malloc(1000);
      void *memory2 = malloc(1000);
      it("free_empty_blocks", [&]() {
        cgc1::details::allocator_block_set_t<cgc1::default_aligned_allocator_t> abs(16, 10000);
        abs.grow_blocks(2);
        abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory1, 992, 16));
        abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory2, 992, 16));
        AssertThat(abs.m_blocks, HasLength(2));
        ::std::vector<cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>> memory_ranges;
        abs.free_empty_blocks(memory_ranges);
        AssertThat(abs.m_blocks, HasLength(2));
        AssertThat(memory_ranges, HasLength(2));
        AssertThat(memory_ranges[0].begin(), Equals(memory1));
        AssertThat(memory_ranges[1].begin(), Equals(memory2));
        AssertThat(memory_ranges[0].end(), Equals(reinterpret_cast<uint8_t *>(memory1) + 992));
        AssertThat(memory_ranges[1].end(), Equals(reinterpret_cast<uint8_t *>(memory2) + 992));
      });
      it("allocator_block_set allocation", [&]() {
        cgc1::details::allocator_block_set_t<cgc1::default_aligned_allocator_t> abs(16, 10000);
        abs.grow_blocks(2);
        abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory1, 992, 16));
        abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory2, 992, 16));
        void *alloc1 = abs.allocate(976);
        void *alloc2 = abs.allocate(976);
        AssertThat(alloc1 != nullptr, IsTrue());
        AssertThat(alloc2 != nullptr, IsTrue());
        AssertThat(abs.allocate(1), Equals((void *)nullptr));
        void *old_alloc1 = alloc1;
        void *old_alloc2 = alloc2;
        abs.destroy(alloc1);
        alloc1 = abs.allocate(976);
        AssertThat(alloc1, Equals(old_alloc1));
        abs.destroy(alloc2);
        abs.destroy(alloc1);
        alloc1 = abs.allocate(976);
        AssertThat(alloc1, Equals(old_alloc1));
        alloc2 = abs.allocate(976);
        AssertThat(alloc2, Equals(old_alloc2));
      });
      free(memory1);
      free(memory2);
    });
    describe("thread_allocator", []() {
      void *memory1 = malloc(1000);
      void *memory2 = malloc(1000);
      it("find_block_set_id", []() {
        AssertThat(cgc1::details::allocator_t<>::this_thread_allocator_t::find_block_set_id(16), Equals((size_t)0));
        AssertThat(cgc1::details::allocator_t<>::this_thread_allocator_t::find_block_set_id(17), Equals((size_t)1));
        AssertThat(cgc1::details::allocator_t<>::this_thread_allocator_t::find_block_set_id(32), Equals((size_t)2));
        AssertThat(cgc1::details::allocator_t<>::this_thread_allocator_t::find_block_set_id(524287),
                   Equals((size_t)15));
        AssertThat(cgc1::details::allocator_t<>::this_thread_allocator_t::find_block_set_id(1000000),
                   Equals((size_t)16));
        AssertThat(cgc1::details::allocator_t<>::this_thread_allocator_t::find_block_set_id(1000000000),
                   Equals((size_t)17));
      });
      free(memory1);
      free(memory2);
    });
    describe("allocator", []() {
      it("test1", []() {
        ::std::unique_ptr<cgc1::details::allocator_t<>> allocator(new cgc1::details::allocator_t<>());
        AssertThat(allocator->initialize(20000, 100000000), IsTrue());
        cgc1::details::allocator_t<>::this_thread_allocator_t ta(*allocator);
        auto &allocator_set = ta.allocator_by_size(55);
        AssertThat(allocator_set.allocator_min_size(), Is().LessThan((size_t)55));
        AssertThat(allocator_set.allocator_max_size(), Is().GreaterThan((size_t)55));
        AssertThat(ta.set_allocator_multiple(20, 50), IsFalse());
        void *alloc1 = ta.allocate(100);
        AssertThat(alloc1 != nullptr, IsTrue());
        void *alloc2 = ta.allocate(100);
        AssertThat(alloc2 != nullptr, IsTrue());
        void *alloc3 = ta.allocate(100);
        AssertThat(alloc3 != nullptr, IsTrue());
        AssertThat(ta.destroy(alloc1), IsTrue());
        AssertThat(ta.allocate(100), Equals(alloc1));
      });
      it("test2", []() {
        ::std::unique_ptr<cgc1::details::allocator_t<>> allocator(new cgc1::details::allocator_t<>());
        AssertThat(allocator->initialize(100000, 100000000), IsTrue());
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
        AssertThat((void *)allocator->current_end(), Equals((void *)allocator->begin()));
      });
      it("test3", []() {
        ::std::unique_ptr<cgc1::details::allocator_t<>> allocator(new cgc1::details::allocator_t<>());
        AssertThat(allocator->initialize(20000, 100000000), IsTrue());
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
        for (int j = 0; j < 30; ++j) {
          ::std::array<cgc1::rebind_vector_t<int, cgc1::cgc_internal_allocator_t<int>>, 5> vecs;
          for (int i = 0; i < 100000; ++i) {
            for (auto &vec : vecs)
              vec.push_back(i);
          }
          for (int i = 0; i < 100000; ++i) {
            for (auto &vec : vecs)
              AssertThat(vec[i], Equals(i));
          }
        }
      });
    });
  });
  describe("Algo", []() {
    it("insert_replace #1", []() {
      std::array<int, 5> in_array{{0, 1, 2, 3, 4}};
      std::array<int, 5> out_array{{0, 2, 3, 4, 55}};
      cgc1::insert_replace(in_array.begin() + 1, in_array.begin() + 4, 55);
      AssertThat(in_array == out_array, IsTrue());
    });
    it("insert_replace #2", []() {
      std::array<int, 5> in_array{{0, 1, 2, 3, 4}};
      std::array<int, 5> out_array{{0, 2, 3, 3, 4}};
      cgc1::insert_replace(in_array.begin() + 1, in_array.begin() + 2, 3);
      AssertThat(in_array == out_array, IsTrue());
    });
    it("insert_replace #3", []() {
      std::array<int, 2> in_array{{0, 1}};
      std::array<int, 2> out_array{{1, 2}};
      cgc1::insert_replace(in_array.begin(), in_array.begin() + 1, 2);
      AssertThat(in_array == out_array, IsTrue());
    });
    it("insert_replace #3", []() {
      std::array<int, 2> in_array{{0, 1}};
      std::array<int, 2> out_array{{0, 5}};
      cgc1::insert_replace(in_array.begin() + 1, in_array.begin() + 1, 5);
      AssertThat(in_array == out_array, IsTrue());
    });
    it("insert_replace #3", []() {
      std::array<int, 2> in_array{{0, 5}};
      std::array<int, 2> out_array{{2, 5}};
      cgc1::insert_replace(in_array.begin(), in_array.begin(), 2);
      AssertThat(in_array == out_array, IsTrue());
    });
  });
  describe("GC", []() {
    it("race condition test", []() {
      for (int j = 0; j < 10; ++j) {
        std::atomic<bool> keep_going;
        keep_going = true;
        auto test_thread = [&keep_going]() {
          CGC1_INITIALIZE_THREAD();
          char *foo = reinterpret_cast<char *>(cgc1::cgc_malloc(100));
          memset(foo, 0, 100);
          {
            CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
            locations.push_back(foo);
          }
          while (keep_going) {
            ::std::stringstream ss;
            ss << foo << ::std::endl;
          };

          foo = nullptr;
          {
            CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
            for (int i = 0; i < 5000; ++i)
              locations.push_back(cgc1::cgc_malloc(100));
          }
          cgc1::cgc_unregister_thread();
        };
        ::std::thread t1(test_thread);
        ::std::thread t2(test_thread);
        for (int i = 0; i < 10; ++i) {
          cgc1::cgc_force_collect();
          cgc1::details::g_gks.wait_for_finalization();
          auto freed_last = cgc1::details::g_gks._d_freed_in_last_collection();
          assert(freed_last.empty());
          AssertThat(freed_last, HasLength(0));
        }
        keep_going = false;
        t1.join();
        t2.join();
        cgc1::cgc_force_collect();
        cgc1::details::g_gks.wait_for_finalization();
        auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
        ::std::sort(locations.begin(), locations.end());
        ::std::sort(last_collect.begin(), last_collect.end());
        for (void *v : locations) {
          assert(::std::find(last_collect.begin(), last_collect.end(), v) != last_collect.end());
          AssertThat(::std::find(last_collect.begin(), last_collect.end(), v) != last_collect.end(), IsTrue());
        }
        locations.clear();
      }
    });
    it("linked list test", []() {
      cgc1::cgc_force_collect();
      cgc1::details::g_gks.wait_for_finalization();
      std::atomic<bool> keep_going;
      keep_going = true;
      auto test_thread = [&keep_going]() {
        CGC1_INITIALIZE_THREAD();
        void **foo = reinterpret_cast<void **>(cgc1::cgc_malloc(100));
        {
          void **bar = foo;
          for (int i = 0; i < 3000; ++i) {
            {
              CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
              locations.push_back(bar);
            }
            memset(bar, 0, 100);
            *bar = cgc1::cgc_malloc(100);
            bar = reinterpret_cast<void **>(*bar);
          }
          {
            CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
            locations.push_back(bar);
          }
        }
        while (keep_going) {
          ::std::stringstream ss;
          ss << foo << ::std::endl;
        }
        cgc1::cgc_unregister_thread();
      };
      ::std::thread t1(test_thread);
      std::this_thread::sleep_for(std::chrono::seconds(1));
      for (int i = 0; i < 100; ++i) {
        cgc1::cgc_force_collect();
        cgc1::details::g_gks.wait_for_finalization();
        auto freed_last = cgc1::details::g_gks._d_freed_in_last_collection();
        assert(freed_last.empty());
        AssertThat(freed_last, HasLength(0));
      }
      keep_going = false;
      t1.join();
      cgc1::cgc_force_collect();
      cgc1::details::g_gks.wait_for_finalization();
      auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
      ::std::sort(locations.begin(), locations.end());
      ::std::sort(last_collect.begin(), last_collect.end());
      for (void *v : locations) {
        assert(::std::find(last_collect.begin(), last_collect.end(), v) != last_collect.end());
        AssertThat(::std::find(last_collect.begin(), last_collect.end(), v) != last_collect.end(), IsTrue());
      }
      locations.clear();
    });
    it("Root test", [](){
      void* memory = cgc1::cgc_malloc(50);
      size_t old_memory = cgc1::hide_pointer(memory);
      cgc1::cgc_add_root(&memory);
      cgc1::cgc_force_collect();
      cgc1::details::g_gks.wait_for_finalization();
      auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
      AssertThat(last_collect, HasLength(0));
      cgc1::cgc_remove_root(&memory);
      memory = nullptr;
      cgc1::cgc_force_collect();
      cgc1::details::g_gks.wait_for_finalization();
      last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
      AssertThat(last_collect, HasLength(1));
      AssertThat(last_collect[0] == cgc1::unhide_pointer(old_memory), IsTrue());
    });
    it("Internal Pointer test", [](){
      uint8_t* memory = reinterpret_cast<uint8_t*>(cgc1::cgc_malloc(50));
      size_t old_memory = cgc1::hide_pointer(memory);
      memory += 1;
      cgc1::cgc_add_root(&memory);
      cgc1::cgc_force_collect();
      cgc1::details::g_gks.wait_for_finalization();
      auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
      AssertThat(last_collect, HasLength(0));
      cgc1::cgc_remove_root(&memory);
      memory = nullptr;
      cgc1::cgc_force_collect();
      cgc1::details::g_gks.wait_for_finalization();
      last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
      AssertThat(last_collect, HasLength(1));
      AssertThat(last_collect[0] == cgc1::unhide_pointer(old_memory), IsTrue());
    });
    it("Finalizers", [](){
      void* memory = cgc1::cgc_malloc(50);
      size_t old_memory = cgc1::hide_pointer(memory);
      std::atomic<bool> finalized = false;
      cgc1::cgc_register_finalizer(memory, [&finalized](auto){finalized = true; });
      cgc1::cgc_register_finalizer(nullptr, [](auto){});
      memory = nullptr;
      cgc1::cgc_force_collect();
      cgc1::details::g_gks.wait_for_finalization();
      auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
      AssertThat(last_collect, HasLength(1));
      AssertThat(last_collect[0] == cgc1::unhide_pointer(old_memory), IsTrue());
      AssertThat((bool)finalized, IsTrue());
    });
    it("Uncollectable", [](){
      void* memory = cgc1::cgc_malloc(50);
      size_t old_memory = cgc1::hide_pointer(memory);
      cgc1::cgc_set_uncollectable(memory, true);
      cgc1::cgc_set_uncollectable(nullptr, true);
      memory = nullptr;
      cgc1::cgc_force_collect();
      cgc1::details::g_gks.wait_for_finalization();
      auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
      AssertThat(last_collect, HasLength(0));
      cgc1::cgc_set_uncollectable(cgc1::unhide_pointer(old_memory), false);
      cgc1::cgc_set_uncollectable(nullptr, false);
      cgc1::cgc_force_collect();
      cgc1::details::g_gks.wait_for_finalization();
      last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
      AssertThat(last_collect, HasLength(1));
      AssertThat(last_collect[0] == cgc1::unhide_pointer(old_memory), IsTrue());
    });
  });
});

int main(int argc, char *argv[])
{
  CGC1_INITIALIZE_THREAD();
  auto ret = bandit::run(argc, argv);
#ifdef _WIN32
  ::std::cin.get();
#endif
  cgc1::cgc_shutdown();
  return ret;
}
