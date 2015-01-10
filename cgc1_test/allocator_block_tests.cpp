#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include "bandit.hpp"
#include <cgc1/aligned_allocator.hpp>
#include "../cgc1/src/slab_allocator.hpp"
#include "../cgc1/src/allocator_block.hpp"
using namespace bandit;
void allocator_block_tests()
{
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
      AssertThat(block.find_address(reinterpret_cast<uint8_t *>(alloc2) + 1) ==
                     cgc1::details::object_state_t::from_object_start(alloc2),
                 IsTrue());
      AssertThat(block.find_address(reinterpret_cast<uint8_t *>(alloc2) + 50) == nullptr, IsTrue());
    });
    free(memory);
  });
}
