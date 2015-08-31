#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include "bandit.hpp"
#include <cgc1/aligned_allocator.hpp>
#include "../cgc1/src/slab_allocator.hpp"
#include "../cgc1/src/allocator_block.hpp"
#include "../cgc1/src/allocator.hpp"
using namespace ::bandit;
using namespace ::cgc1::literals;

void allocator_block_set_tests()
{
  describe("allocator_block_set", []() {
    void *memory1 = malloc(1000);
    void *memory2 = malloc(1000);
    void *memory3 = malloc(1000);
    void *memory4 = malloc(1000);
    it("free_empty_blocks", [&]() {
      // setup an allocator with two blocks to test free_empty_blocks.
      using abs_type = cgc1::details::allocator_block_set_t<cgc1::default_aligned_allocator_t>;
      abs_type abs(16, 10000);
      abs.grow_blocks(2);
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory1, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory2, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      AssertThat(abs.m_blocks, HasLength(2));
      ::std::vector<cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>> memory_ranges;
      // this should be a noop.
      abs.free_empty_blocks(
          [&memory_ranges](typename abs_type::allocator_block_type &&block) { memory_ranges.push_back(::std::move(block)); },
          []() {}, []() {}, [](auto, auto, auto) {}, 2);
      AssertThat(abs.m_blocks, HasLength(2));
      AssertThat(memory_ranges, HasLength(0));
      // this should actually remove the empty blocks.
      abs.free_empty_blocks(
          [&memory_ranges](typename abs_type::allocator_block_type &&block) { memory_ranges.push_back(::std::move(block)); },
          []() {}, []() {}, [](auto, auto, auto) {}, 0);
      AssertThat(abs.m_blocks, HasLength(0));
      AssertThat(memory_ranges, HasLength(2));
      // test that moved blocks have correct data.
      AssertThat(memory_ranges[1].begin(), Equals(memory1));
      AssertThat(memory_ranges[0].begin(), Equals(memory2));
      AssertThat(memory_ranges[1].end(), Equals(reinterpret_cast<uint8_t *>(memory1) + 992));
      AssertThat(memory_ranges[0].end(), Equals(reinterpret_cast<uint8_t *>(memory2) + 992));
      // done
    });
    it("allocator_block_set allocation", [&]() {
      // setup an allocator with two blocks for testing.
      cgc1::details::allocator_block_set_t<cgc1::default_aligned_allocator_t> abs(16, 10000);
      abs.grow_blocks(2);
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory1, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory2, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      // allocator two memory locations with max size.
      void *alloc1 = abs.allocate(976);
      void *alloc2 = abs.allocate(976);
      AssertThat(alloc1 != nullptr, IsTrue());
      AssertThat(alloc2 != nullptr, IsTrue());
      // test that trying to allocate more memory fails.
      AssertThat(abs.allocate(1), Equals(static_cast<void *>(nullptr)));
      void *old_alloc1 = alloc1;
      void *old_alloc2 = alloc2;
      // try to destroy and recreate some memory.
      abs.destroy(alloc1);
      alloc1 = abs.allocate(976);
      AssertThat(alloc1, Equals(old_alloc1));
      // try to destroy and recreate both memory addresses.
      abs.destroy(alloc2);
      abs.destroy(alloc1);
      alloc1 = abs.allocate(976);
      AssertThat(alloc1, Equals(old_alloc1));
      alloc2 = abs.allocate(976);
      AssertThat(alloc2, Equals(old_alloc2));
      // done
    });
    it("allocator_block_set_add_block", [&]() {
      // this test is testing ordering of available blocks.
      // setup an allocator with three blocks. testing.
      cgc1::details::allocator_block_set_t<cgc1::default_aligned_allocator_t> abs(16, 10000);
      abs.grow_blocks(3);
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory1, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      // most recently added block should not be in available blocks.
      AssertThat(abs.m_available_blocks, HasLength(0));
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory2, 512, 16,
                                                                                        cgc1::details::c_infinite_length));
      AssertThat(abs.m_available_blocks, HasLength(1));
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory3, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      // available blocks should be in sorted order.
      AssertThat(abs.m_blocks, HasLength(3));
      AssertThat(abs.m_available_blocks, HasLength(2));
      AssertThat(abs.m_available_blocks[0].first, Equals(static_cast<size_t>(496)));
      AssertThat(abs.m_available_blocks[0].second, Equals(&abs.m_blocks[1]));
      AssertThat(abs.m_available_blocks[1].first, Equals(static_cast<size_t>(976)));
      AssertThat(abs.m_available_blocks[1].second, Equals(&abs.m_blocks[0]));
      // done
    });
    it("allocator_block_set_remove_block", [&]() {
      // setup an allocator with three blocks for testing.
      cgc1::details::allocator_block_set_t<cgc1::default_aligned_allocator_t> abs(16, 10000);
      abs.grow_blocks(3);
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory1, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory2, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory3, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      AssertThat(abs.m_available_blocks, HasLength(2));
      AssertThat(abs.m_available_blocks[0].second, Equals(&*(abs.m_blocks.begin())));
      AssertThat(abs.m_available_blocks[1].second, Equals(&*(abs.m_blocks.begin() + 1)));
      abs.remove_block(abs.m_blocks.begin() + 1, []() {}, []() {}, [](auto, auto, auto) {});
      AssertThat(abs.m_available_blocks, HasLength(1));
      AssertThat(abs.m_available_blocks[0].second, Equals(&*(abs.m_blocks.begin())));
    });

    it("allocator_block_set_destroy_rotate", [&]() {
      // we want to test rotate functionality in destroy.
      // setup an allocator with three blocks for testing.
      cgc1::details::allocator_block_set_t<cgc1::default_aligned_allocator_t> abs(16, 10000);
      abs.grow_blocks(4);
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory1, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory2, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory3, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory3, 992, 16,
                                                                                        cgc1::details::c_infinite_length));

      void *alloc1 = abs.allocate(900);
      void *alloc2 = abs.allocate(800);
      void *alloc3 = abs.allocate(700);

      AssertThat(abs.m_available_blocks, HasLength(3_sz));
      AssertThat(abs.m_available_blocks[0].second, Equals(&abs.m_blocks[0]));
      AssertThat(abs.m_available_blocks[1].second, Equals(&abs.m_blocks[1]));
      AssertThat(abs.m_available_blocks[2].second, Equals(&abs.m_blocks[2]));

      abs.destroy(alloc2);
      AssertThat(abs.m_available_blocks, HasLength(3_sz));
      AssertThat(abs.m_available_blocks[0].second, Equals(&abs.m_blocks[0]));
      AssertThat(abs.m_available_blocks[1].second, Equals(&abs.m_blocks[2]));
      // catch writing pair to wrong place (ACTUAL BUG).
      AssertThat(abs.m_available_blocks[2].first, Equals(976_sz));
      AssertThat(abs.m_available_blocks[2].second, Equals(&abs.m_blocks[1]));

      abs.destroy(alloc1);
      abs.destroy(alloc2);
      abs.destroy(alloc3);

    });

    free(memory1);
    free(memory2);
    free(memory3);
    free(memory4);
  });
}
