#include <mcppalloc_sparse/mcppalloc_sparse.hpp>
#include <mcppalloc_utils/bandit.hpp>
#include <mcppalloc_utils/aligned_allocator.hpp>
#include <mcppalloc_sparse/allocator_block.hpp>
using namespace bandit;
template <>
::mcppalloc::details::user_data_base_t mcppalloc::sparse::details::allocator_block_t<
    mcppalloc::default_allocator_policy_t<mcppalloc::aligned_allocator_t<void, 8ul>>>::s_default_user_data{};
void allocator_block_tests()
{
  describe("Block", []() {
    void *memory = malloc(200);
    using policy = ::mcppalloc::default_allocator_policy_t<::mcppalloc::default_aligned_allocator_t>;
    using allocator_block_type = typename ::mcppalloc::sparse::details::allocator_block_t<policy>;

    using object_state_type = typename allocator_block_type::object_state_type;
    const auto aligned_header_size =
        mcppalloc::align(sizeof(object_state_type), allocator_block_type::allocator_policy_type::cs_minimum_alignment);
    auto memory_size = (aligned_header_size + 16) * 3;
    allocator_block_type block(memory, memory_size, 16, ::mcppalloc::c_infinite_length);
    void *alloc1 = nullptr;
    void *alloc2 = nullptr;
    void *alloc3 = nullptr;
    void *alloc4 = nullptr;
    it("alloc1", [&]() {
      alloc1 = block.allocate(15);
      AssertThat(alloc1, !Equals(static_cast<void *>(nullptr)));
      AssertThat(object_state_type::from_object_start(alloc1)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc1)->not_available(), IsTrue());
    });
    it("alloc2", [&]() {
      alloc2 = block.allocate(15);
      AssertThat(alloc2, !Equals(static_cast<void *>(nullptr)));
      AssertThat(object_state_type::from_object_start(alloc2)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc2)->not_available(), IsTrue());
    });
    it("alloc3", [&]() {
      alloc3 = block.allocate(15);
      AssertThat(alloc3, !Equals(static_cast<void *>(nullptr)));
      AssertThat(object_state_type::from_object_start(alloc3)->next_valid(), IsFalse());
      AssertThat(object_state_type::from_object_start(alloc3)->not_available(), IsTrue());
      auto alloc3_state = object_state_type::from_object_start(alloc3);
      AssertThat(alloc3_state->next() == reinterpret_cast<object_state_type *>(block.end()), IsTrue());
    });
    it("alloc4", [&]() {
      alloc4 = block.allocate(15);
      AssertThat(alloc4, Equals(static_cast<void *>(nullptr)));
    });
    it("re-alloc 3", [&]() {
      auto old_alloc3 = alloc3;
      block.destroy(alloc3);
      alloc3 = block.allocate(15);
      AssertThat(!!alloc3, IsTrue());
      AssertThat(alloc3, Equals(old_alloc3));
      AssertThat(object_state_type::from_object_start(alloc3)->next_valid(), IsFalse());
      AssertThat(object_state_type::from_object_start(alloc3)->not_available(), IsTrue());
    });
    it("re-alloc 2", [&]() {
      auto old_alloc2 = alloc2;
      block.destroy(alloc2);
      alloc2 = block.allocate(15);
      AssertThat(alloc2, Equals(old_alloc2));
      AssertThat(object_state_type::from_object_start(alloc2)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc2)->not_available(), IsTrue());
    });
    it("re-alloc 2&3", [&]() {
      AssertThat(!!alloc3, IsTrue());
      auto old_alloc3 = alloc3;
      auto old_alloc2 = alloc2;
      block.destroy(alloc2);
      block.destroy(alloc3);
      AssertThat(block.m_free_list, HasLength(1));
      alloc2 = block.allocate(15);
      AssertThat(alloc2, Equals(old_alloc2));
      AssertThat(object_state_type::from_object_start(alloc2)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc2)->not_available(), IsTrue());
      alloc3 = block.allocate(15);
      AssertThat(alloc3, Equals(old_alloc3));
      AssertThat(object_state_type::from_object_start(alloc3)->next_valid(), IsFalse());
      AssertThat(object_state_type::from_object_start(alloc3)->not_available(), IsTrue());
    });
    it("re-alloc 2&3 #2", [&]() {
      AssertThat(!!alloc3, IsTrue());
      auto old_alloc3 = alloc3;
      auto old_alloc2 = alloc2;
      auto alloc3_state = object_state_type::from_object_start(alloc3);
      AssertThat(alloc3_state->next() == reinterpret_cast<object_state_type *>(block.end()), IsTrue());
      block.destroy(alloc3);
      AssertThat(alloc3_state->next_valid(), !IsTrue());
      AssertThat(alloc3_state->next() == reinterpret_cast<object_state_type *>(block.end()), IsTrue());
      AssertThat(alloc3_state->in_use(), !IsTrue());
      block.destroy(alloc2);
      AssertThat(block.m_free_list, HasLength(0));
      alloc2 = block.allocate(15);
      AssertThat(alloc2, Equals(old_alloc2));
      AssertThat(object_state_type::from_object_start(alloc2)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc2)->not_available(), IsTrue());
      alloc3 = block.allocate(15);
      AssertThat(alloc3, Equals(old_alloc3));
      AssertThat(object_state_type::from_object_start(alloc3)->next_valid(), IsFalse());
      AssertThat(object_state_type::from_object_start(alloc3)->not_available(), IsTrue());
    });
    it("realloc 1&2", [&]() {
      auto old_alloc2 = alloc2;
      auto old_alloc1 = alloc1;
      block.destroy(alloc1);
      block.destroy(alloc2);
      AssertThat(block.m_free_list, HasLength(2));
      alloc2 = block.allocate(15);
      AssertThat(alloc2, Equals(old_alloc2));
      AssertThat(object_state_type::from_object_start(alloc2)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc2)->not_available(), IsTrue());
      alloc1 = block.allocate(15);
      AssertThat(alloc1, Equals(old_alloc1));
      AssertThat(object_state_type::from_object_start(alloc1)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc1)->not_available(), IsTrue());
    });
    it("realloc 1&2 #2", [&]() {
      auto old_alloc2 = alloc2;
      auto old_alloc1 = alloc1;
      block.destroy(alloc2);
      block.destroy(alloc1);
      AssertThat(block.m_free_list, HasLength(1));
      alloc1 = block.allocate(15);
      AssertThat(alloc1, Equals(old_alloc1));
      AssertThat(object_state_type::from_object_start(alloc1)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc1)->not_available(), IsTrue());
      AssertThat(block.m_free_list, HasLength(1));
      alloc2 = block.allocate(15);
      AssertThat(alloc2, Equals(old_alloc2));
      AssertThat(object_state_type::from_object_start(alloc2)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc2)->not_available(), IsTrue());
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
      AssertThat(object_state_type::from_object_start(alloc1)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc1)->not_available(), IsTrue());
      AssertThat(block.m_free_list, HasLength(0));
      block.destroy(alloc1);
      AssertThat(block.m_free_list, HasLength(1));
      alloc1 = block.allocate(15);
      AssertThat(alloc1, Equals(old_alloc1));
      AssertThat(object_state_type::from_object_start(alloc1)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc1)->not_available(), IsTrue());
      AssertThat(block.m_free_list, HasLength(1));
      alloc2 = block.allocate(15);
      AssertThat(alloc2, Equals(old_alloc2));
      AssertThat(object_state_type::from_object_start(alloc2)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc2)->not_available(), IsTrue());
      AssertThat(block.m_free_list, HasLength(0));
    });
    it("Fail realloc 1&2 #2", [&]() {
      auto old_alloc2 = alloc2;
      auto old_alloc1 = alloc1;
      block.destroy(alloc2);
      block.destroy(alloc1);
      AssertThat(block.m_free_list, HasLength(1));
      AssertThat(block.allocate(17 + 2 * aligned_header_size), Equals(static_cast<void *>(nullptr)));
      alloc1 = block.allocate(15);
      AssertThat(alloc1, Equals(old_alloc1));
      AssertThat(object_state_type::from_object_start(alloc1)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc1)->not_available(), IsTrue());
      AssertThat(block.m_free_list, HasLength(1));
      alloc2 = block.allocate(15);
      AssertThat(alloc2, Equals(old_alloc2));
      AssertThat(object_state_type::from_object_start(alloc2)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc2)->not_available(), IsTrue());
      AssertThat(block.m_free_list, HasLength(0));
    });
    it("fail massive alloc", []() {
      void *memory2 = malloc(1000);
      allocator_block_type block2(memory2, 96, 16, ::mcppalloc::c_infinite_length);
      AssertThat(block2.allocate(985), Equals(static_cast<void *>(nullptr)));
      free(memory2);
    });
    it("empty", []() {
      void *memory2 = malloc(1000);
      allocator_block_type block2(memory2, 992, 16, ::mcppalloc::c_infinite_length);
      AssertThat(block2.empty(), IsTrue());
      auto m = block2.allocate(500);
      AssertThat(block2.empty(), IsFalse());
      block2.destroy(m);
      AssertThat(block2.empty(), IsTrue());
      free(memory2);
    });
    it("collect", [&]() {
      AssertThat(!!alloc3, IsTrue());
      auto old_alloc3 = alloc3;
      auto old_alloc2 = alloc2;
      auto old_alloc1 = alloc1;
      block.destroy(alloc1);
      block.destroy(alloc2);
      AssertThat(block.m_free_list, HasLength(2));
      size_t num_quasifreed = 0;
      block.collect(num_quasifreed);
      AssertThat(block.m_free_list, HasLength(1));
      block.destroy(alloc3);
      AssertThat(block.m_free_list, HasLength(1));
      num_quasifreed = 0;
      block.collect(num_quasifreed);
      AssertThat(block.m_free_list, HasLength(0));
      alloc1 = block.allocate(15);
      AssertThat(alloc1, Equals(old_alloc1));
      AssertThat(object_state_type::from_object_start(alloc1)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc1)->not_available(), IsTrue());
      alloc2 = block.allocate(15);
      AssertThat(alloc2, Equals(old_alloc2));
      AssertThat(object_state_type::from_object_start(alloc2)->next_valid(), IsTrue());
      AssertThat(object_state_type::from_object_start(alloc2)->not_available(), IsTrue());
      alloc3 = block.allocate(15);
      AssertThat(alloc3, Equals(old_alloc3));
      AssertThat(object_state_type::from_object_start(alloc3)->next_valid(), IsFalse());
      AssertThat(object_state_type::from_object_start(alloc3)->not_available(), IsTrue());
    });
    it("find", [&]() {
      AssertThat(block.find_address(alloc2) == object_state_type::from_object_start(alloc2), IsTrue());
      AssertThat(block.find_address(reinterpret_cast<uint8_t *>(alloc2) + 1) == object_state_type::from_object_start(alloc2),
                 IsTrue());
      AssertThat(block.find_address(reinterpret_cast<uint8_t *>(alloc2) + 34 + aligned_header_size) == nullptr, IsTrue());
    });
    free(memory);
  });
}
