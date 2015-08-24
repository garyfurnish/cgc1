#pragma once
#include "object_state.hpp"
#include "allocator_block.hpp"
namespace cgc1
{
  namespace details
  {
    using memory_range_t = ::std::pair<void *, void *>;
    /**
     * \brief This is a set of allocator blocks with the same minimum and maximum allocation size.
    **/
    template <typename Allocator = ::std::allocator<void>, typename Allocator_Block_User_Data = user_data_base_t>
    class allocator_block_set_t
    {
    public:
      using allocator = Allocator;
      using allocator_block_user_data_type = Allocator_Block_User_Data;
      using allocator_block_type = allocator_block_t<allocator, allocator_block_user_data_type>;
      using allocator_block_vector_t = rebind_vector_t<allocator_block_type, allocator>;
      using sized_block_ref_t = typename ::std::pair<size_t, allocator_block_type *>;
      using allocator_block_reference_vector_t = rebind_vector_t<sized_block_ref_t, allocator>;

      explicit allocator_block_set_t() = default;
      allocator_block_set_t(const allocator_block_set_t<allocator, allocator_block_user_data_type> &) = delete;
      allocator_block_set_t(allocator_block_set_t<allocator, allocator_block_user_data_type> &&abs) = default;
      allocator_block_set_t &operator=(const allocator_block_set_t<allocator, allocator_block_user_data_type> &) = delete;
      allocator_block_set_t &operator=(allocator_block_set_t<allocator, allocator_block_user_data_type> &&) = default;
      /**
       * \brief Constructor
       * @param allocator_min_size Minimum allocation size.
       * @param allocator_max_size Maximum allocation size.
      **/
      allocator_block_set_t(size_t allocator_min_size, size_t allocator_max_size);
      /**
       * \brief Set minimum and maximum allocation size.
       * This should not be called after first use.
       * @param min Minimum allocation size.
       * @param max Maximum allocation size.
      **/
      void _set_allocator_sizes(size_t min, size_t max);
      /**
       * \brief Return the minimum allocation size.
      **/
      size_t allocator_min_size() const;
      /**
       * \brief Return the maximum object allocation size.
      **/
      size_t allocator_max_size() const;
      /**
       * \brief Return the number of blocks in the set.
      **/
      size_t size() const;
      /**
       * \brief Regenerate available blocks in case it is stale.
       * Also should be called if m_blocks may have changed locations because of allocation.
      **/
      void regenerate_available_blocks();
      /**
       * \brief Collect all blocks in set.
      **/
      void collect();
      /**
       * \brief Allocate memory of given size, return nullptr if not possible in existing blocks.
       * @param sz Size to allocate.
       * @return A pointer to allocated memory, nullptr on failure.
      **/
      void *allocate(size_t sz);
      /**
       * \brief Destroy memory.
       * @return True if this block set allocated the memory and thus destroyed it, false otherwise.
      **/
      bool destroy(void *v);
      /**
       * \brief Add a block to the set.
      **/
      void add_block(allocator_block_type &&block);
      /**
       * \brief Add an empty block to the set.
      **/
      auto add_block() -> allocator_block_type &;
      /**
       * \brief Remove a block from the set.
      **/
      void remove_block(typename allocator_block_vector_t::iterator it);
      /**
       * \brief Return true if add_block would cause the container of blocks to move in memory, false otherwise.
      **/
      bool add_block_is_safe() const;
      /**
       * \brief Grow the capacity of m_blocks.  Return the offset by which it moved.
       * Thus subtract offset from m_blocks[i] to get the old position.
       * @param sz If provided, the number of blocks to reserve.
      **/
      size_t grow_blocks(size_t sz = 0);
      /**
       * \brief Return reference to last added block.
      **/
      auto last_block() -> allocator_block_type &;
      /**
       * \brief Return reference to last added block.
      **/
      auto last_block() const -> const allocator_block_type &;
      /**
       * \brief Push all empty block memory ranges onto container t and then remove them.
       * @param l Function to call on removed blocks (called multiple times with r val block ref).
       * @param min_to_leave Minimum number of free blocks to leave in this set.
      **/
      template <typename L>
      void free_empty_blocks(L &&l, size_t min_to_leave = 0);

      /**
       * \brief Return the number of memory addresses destroyed since last free empty blocks operation.
       **/
      auto num_destroyed_since_last_free() const noexcept -> size_t;
      /**

       **/
      allocator_block_type *m_back = nullptr;
      /**
       * \brief All blocks.
      **/
      allocator_block_vector_t m_blocks;
      /**
       * \brief Blocks that are available for placement.
      **/
      allocator_block_reference_vector_t m_available_blocks;
      /**
       * \brief Do internal verification.
      **/
      void _verify() const;
      /**
       * \brief Do maintance on thread associated blocks.
       * (coalescing, etc)
      **/
      void _do_maintenance();

    private:
      /**
       * \brief Minimum allocation size.
      **/
      size_t m_allocator_min_size = 0;
      /**
       * \brief Maximum allocation size.
      **/
      size_t m_allocator_max_size = 0;
      /*
       * \brief Number of memory addresses destroyed since last free empty blocks operation.
       **/
      size_t m_num_destroyed_since_free = 0;
    };
  }
}
#include "allocator_block_set_impl.hpp"
