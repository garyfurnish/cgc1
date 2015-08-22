#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include "bandit.hpp"
#include <cgc1/aligned_allocator.hpp>
#include "../cgc1/src/slab_allocator.hpp"
#include "../cgc1/src/allocator_block.hpp"
#include "../cgc1/src/allocator.hpp"
using namespace bandit;

void allocator_block_set_tests()
{
  describe("allocator_block_set", []() {
    void *memory1 = malloc(1000);
    void *memory2 = malloc(1000);
    it("free_empty_blocks", [&]() {
      cgc1::details::allocator_block_set_t<cgc1::default_aligned_allocator_t> abs(16, 10000);
      abs.grow_blocks(2);
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory1, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory2, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      AssertThat(abs.m_blocks, HasLength(2));
      ::std::vector<cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>> memory_ranges;
      //      abs.free_empty_blocks(memory_ranges,2);
      AssertThat(abs.m_blocks, HasLength(2));
      abs.free_empty_blocks(memory_ranges);
      AssertThat(abs.m_blocks, HasLength(0));
      AssertThat(memory_ranges, HasLength(2));
      AssertThat(memory_ranges[0].begin(), Equals(memory1));
      AssertThat(memory_ranges[1].begin(), Equals(memory2));
      AssertThat(memory_ranges[0].end(), Equals(reinterpret_cast<uint8_t *>(memory1) + 992));
      AssertThat(memory_ranges[1].end(), Equals(reinterpret_cast<uint8_t *>(memory2) + 992));
    });
    it("allocator_block_set allocation", [&]() {
      cgc1::details::allocator_block_set_t<cgc1::default_aligned_allocator_t> abs(16, 10000);
      abs.grow_blocks(2);
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory1, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      abs.add_block(cgc1::details::allocator_block_t<cgc1::default_aligned_allocator_t>(memory2, 992, 16,
                                                                                        cgc1::details::c_infinite_length));
      void *alloc1 = abs.allocate(976);
      void *alloc2 = abs.allocate(976);
      AssertThat(alloc1 != nullptr, IsTrue());
      AssertThat(alloc2 != nullptr, IsTrue());
      AssertThat(abs.allocate(1), Equals(static_cast<void *>(nullptr)));
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
}
