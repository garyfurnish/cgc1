#pragma once
namespace mcppalloc
{
  namespace sparse
  {
    namespace details
    {
      /**
       * \brief This class stores per allocator block set data for a thread allocator.
       *
       * We do not store this information inside of the abs because it is only useful
       * inside the context of a thread allocator and therefore separation of concerns
       * says it should not be stored in the abs.
      **/
      class thread_allocator_abs_data_t
      {
      public:
        thread_allocator_abs_data_t() noexcept = default;
        thread_allocator_abs_data_t(uint32_t allocator_multiple, uint32_t max_blocks_before_recycle);
        thread_allocator_abs_data_t(const thread_allocator_abs_data_t &) = default;
        thread_allocator_abs_data_t(thread_allocator_abs_data_t &&) = default;
        thread_allocator_abs_data_t &operator=(const thread_allocator_abs_data_t &) = default;
        thread_allocator_abs_data_t &operator=(thread_allocator_abs_data_t &&) = default;
        /**
        * \brief Return the allocator multiple for this block set.
        **/
        auto allocator_multiple() const noexcept -> uint32_t;
        /**
         * \brief Return the maximum blocks allocated before recycling for this block set.
        **/
        auto max_blocks_before_recycle() const noexcept -> uint32_t;
        /**
         * \brief Set the allocator multiple for this block set.
        **/
        void set_allocator_multiple(uint32_t multiple);
        /**
         * \brief Set the maximum blocks before recycle for this block set.
        **/
        void set_max_blocks_before_recycle(uint32_t blocks);

      private:
        /**
         * \brief Allocator multiple (power of 2) for referenced abs.
        **/
        uint32_t m_allocator_multiple = 0;
        /**
         * \brief Maximum number of blocks this abs should contain before recycling some back to global level.
        **/
        uint32_t m_max_blocks_before_recycle = 1;
      };
    }
  }
}
#include "thread_allocator_abs_data_impl.hpp"
