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
     *
     * This stores lists of allocator blocks for various sizes of allocations.
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
      allocator_block_set_t(allocator_block_set_t<allocator, allocator_block_user_data_type> &&abs) = delete;
      allocator_block_set_t &operator=(const allocator_block_set_t<allocator, allocator_block_user_data_type> &) = delete;
      allocator_block_set_t &operator=(allocator_block_set_t<allocator, allocator_block_user_data_type> &&) = delete;
      /**
       * \brief Constructor
       *
       * @param allocator_min_size Minimum allocation size.
       * @param allocator_max_size Maximum allocation size.
      **/
      allocator_block_set_t(size_t allocator_min_size, size_t allocator_max_size);
      /**
       * \brief Set minimum and maximum allocation size.
       *
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
       *
       * Also should be called if m_blocks may have changed locations because of allocation.
      **/
      void regenerate_available_blocks();
      /**
       * \brief Collect all blocks in set.
      **/
      void collect();
      /**
       * \brief Allocate memory of given size, return nullptr if not possible in existing blocks.
       *
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
       *
       * @param lock_func Functional with no args called before modifing blocks.
       * @param unlock_func Functional with no args called after modifying blocks.
       * @param move_func Functional to call on moved blocks.
      **/
      template <typename Lock_Functional, typename Unlock_Functional, typename Move_Functional>
      auto add_block(allocator_block_type &&block,
                     Lock_Functional &&lock_func,
                     Unlock_Functional &&unlock_func,
                     Move_Functional &&move_func) -> allocator_block_type &;
      /**
       * \brief Add a block to the set.
      **/
      auto add_block(allocator_block_type &&block) -> allocator_block_type &;

      /**
       * \brief Remove a block from the set.
       *
       * The functional on_move is a functional of the form (begin,end,offset) that is called on blocks that are moved.
       * param it Position to remove.
       * @param lock_func Functional with no args called before modifing blocks.
       * @param unlock_func Functional with no args called after modifying blocks.
       * @param move_func Functional to call on moved blocks.
       **/
      template <typename Lock_Functional, typename Unlock_Functional, typename Move_Functional>
      void remove_block(typename allocator_block_vector_t::iterator it,
                        Lock_Functional &&lock_func,
                        Unlock_Functional &&unlock_func,
                        Move_Functional &&move_func);
      /**
       * \brief Return true if add_block would cause the container of blocks to move in memory, false otherwise.
      **/
      bool add_block_is_safe() const;
      /**
       * \brief Grow the capacity of m_blocks.  Return the offset by which it moved.
       *
       * Thus subtract offset from m_blocks[i] to get the old position.
       * @param sz If provided, the number of blocks to reserve.
      **/
      size_t grow_blocks(size_t sz = 0);
      /**
       * \brief Return reference to last added block.
      **/
      auto last_block() noexcept -> allocator_block_type &;
      /**
       * \brief Return reference to last added block.
      **/
      auto last_block() const noexcept -> const allocator_block_type &;
      /**
       * \brief Push all empty block memory ranges onto container t and then remove th
       *
       * The on_move is a functional of the form (begin,end,offset) that is called on blocks that are moved.
       * @param l Function to call on removed blocks (called multiple times with r val block ref).
       * @param lock_func Functional with no args called before modifing blocks.
       * @param unlock_func Functional with no args called after modifying blocks.
       * @param move_func Functional to call on moved blocks.
       * @param min_to_leave Minimum number of free blocks to leave in this set.
      **/
      template <typename L, typename Lock_Functional, typename Unlock_Functional, typename Move_Functional>
      void free_empty_blocks(L &&l,
                             Lock_Functional &&lock_func,
                             Unlock_Functional &&unlock_func,
                             Move_Functional &&move_func,
                             size_t min_to_leave = 0);

      /**
       * \brief Return the number of memory addresses destroyed since last free empty blocks operation.
       **/
      auto num_destroyed_since_last_free() const noexcept -> size_t;
      /**
       * \brief Do internal verification.
      **/
      void _verify() const;
      /**
       * \brief Do maintance on thread associated blocks.
       *
       * Coalescing, etc happens here.
      **/
      void _do_maintenance();

    private:
      allocator_block_type *m_last_block = nullptr;

    public:
      /**
       * \brief Blocks that are available for placement.
       *
       * This is sorted by allocation size available.
       * It does not contain a pointer to the last block.
       * So if last block keeps getting hit, it is not necessary to recalculate memory free.
       * First part of an element is memory available in that block.
       * Second part is a pointer to the block.
      **/
      allocator_block_reference_vector_t m_available_blocks;
      /**
       * \brief All blocks.
      **/
      allocator_block_vector_t m_blocks;

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
