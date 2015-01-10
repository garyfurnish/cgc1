#pragma once
#include "object_state.hpp"
#include "allocator_block.hpp"
namespace cgc1
{
  namespace details
  {
    using memory_range_t = ::std::pair<void *, void *>;
    /**
    This is a set of allocator blocks with the same minimum and maximum allocation size.
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
      Constructor
      @param allocator_min_size Minimum allocation size.
      @param allocator_max_size Maximum allocation size.
      **/
      allocator_block_set_t(size_t allocator_min_size, size_t allocator_max_size);
      /**
      Set minimum and maximum allocation size.
      This should not be called after first use.
      **/
      void _set_allocator_sizes(size_t min, size_t max);
      /**
      Return the minimum allocation size.
      **/
      size_t allocator_min_size() const;
      /**
      Return the maximum object allocation size.
      **/
      size_t allocator_max_size() const;
      /**
      Return the number of blocks in the set.
      **/
      size_t size() const;
      /**
      Regenerate available blocks in case it is stale.
      Also should be called if m_blocks may have changed locations because of allocation.
      **/
      void regenerate_available_blocks();
      /**
      Collect all blocks in set.
      **/
      void collect();
      /**
      Allocate memory of given size, return nullptr if not possible in existing blocks.
      **/
      void *allocate(size_t sz);
      /**
      Destroy memory.
      @return True if this block set allocated the memory and thus destroyed it, false otherwise.
      **/
      bool destroy(void *v);
      /**
      Add a block to the set.
      **/
      void add_block(allocator_block_type &&block);
      /**
      Remove a block from the set.
      **/
      void remove_block(typename allocator_block_vector_t::iterator it);
      /**
      Return if add_block would cause the container of blocks to move in memory.
      **/
      bool add_block_is_safe() const;
      /**
      Grow the capacity of m_blocks.  Return the offset by which it moved.
      Thus subtract offset from m_blocks[i] to get the old position.
      @param sz If provided, the number of blocks to reserve.
      **/
      size_t grow_blocks(size_t sz = 0);
      /**
      Return reference to last added block.
      **/
      allocator_block_type &last_block();
      /**
      Push all empty block memory ranges onto container t and then remove them.
      **/
      template <typename T>
      void free_empty_blocks(T &t);
      allocator_block_type *m_back = nullptr;
      /**
      All blocks.
      **/
      allocator_block_vector_t m_blocks;
      /**
      Blocks that are available for placement.
      **/
      allocator_block_reference_vector_t m_available_blocks;
      /**
      Do internal verification.
      **/
      void _verify() const;
      /**
      Do maintance on thread associated blocks.
      (coalescing, etc)
      **/
      void _do_maintenance();

    private:
      /**
      Minimum allocation size.
      **/
      size_t m_allocator_min_size = 0;
      /**
      Maximum allocation size.
      **/
      size_t m_allocator_max_size = 0;
    };
  }
}
#include "allocator_block_set_impl.hpp"
