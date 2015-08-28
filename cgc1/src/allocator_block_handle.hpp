#pragma once
namespace cgc1
{
  namespace details
  {
    /**
     * \brief Structure used to store data about an allocation block in an allocator.
    **/
    template <typename Global_Allocator>
    struct allocator_block_handle_t {
      using global_allocator_t = Global_Allocator;
      using block_type = typename global_allocator_t::block_type;
      /**
       * \brief Constructor.
       *
       * @param ta Owning thread allocator, may be nullptr for global.
       * @param block Block address.
       * @param begin Beginning of block data.
       **/
      void initialize(typename global_allocator_t::this_thread_allocator_t *ta, block_type *block, uint8_t *begin)
      {
	m_thread_allocator = ta;
	m_block = block;
	m_begin = begin;
      }
      bool operator==(const allocator_block_handle_t &b) const
      {
        return m_thread_allocator == b.m_thread_allocator && m_block == b.m_block;
      }
      /**
       * \brief Thread allocator that currently owns this handle.
       *
       * May be nullptr.
      **/
      typename global_allocator_t::this_thread_allocator_t *m_thread_allocator;
      /**
       * \brief Address of block for this handle.
       *
       * This does not own the block.
      **/
      block_type *m_block;
      /**
       * \brief Start location of block data.
       *
       * Since allocator_blocks may be temporarily inconsistent during a move operation, we cash their beginning location.
      **/
      uint8_t *m_begin;
    };
  }
}
