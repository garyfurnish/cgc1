#pragma once
#include <cgc1/concurrency.hpp>
#include "object_state.hpp"
#include "allocator_block_set.hpp"
#include "thread_allocator_abs_data.hpp"
#include <cgc1/posix_slab.hpp>
#include <array>
namespace cgc1
{
  namespace details
  {
    /**
     * \brief Treating the pair as a beginning and end, return the size of the memory block between them.
    **/
    inline ::std::ptrdiff_t size(const ::std::pair<uint8_t *, uint8_t *> &pair);
    /**
     * \brief Treating the pair as a beginning and end, return the positive size of the memory block between them.
    **/
    inline size_t size_pos(const ::std::pair<uint8_t *, uint8_t *> &pair);
    /**
     * \brief Output the memory pair as a stirng to the stream.
    **/
    template <typename T>
    void print_memory_pair(T &os, const ::std::pair<uint8_t *, uint8_t *> &pair);
    /**
     * \brief Output the memory pair to the string.
    **/
    inline ::std::string to_string(const ::std::pair<uint8_t *, uint8_t *> &pair);
    template <typename Allocator, typename Traits>
    class allocator_t;
    /**
     * \brief Per thread allocator for a global allocator.
    **/
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    class thread_allocator_t
    {
    public:
      /**
       * \brief Type of allocator to use for internal operations.
      **/
      using allocator = Allocator;
      /**
       * \brief Type of global allocator that created this thread allocator.
      **/
      using global_allocator = Global_Allocator;
      /**
       * \brief Allocator traits for global allocator.
      **/
      using allocator_traits = Allocator_Traits;
      /**
       * \brief Allocator block set for this thread allocator.
      **/
      using this_allocator_block_set_t =
          allocator_block_set_t<allocator, typename allocator_traits::allocator_block_user_data_type>;
      /**
       * \brief Type for destroy threshold.
       *
       * This is here because maybe in the future someone wants to support uint32_t.
       * However, uint16_t is better for cache locality.
      **/
      using destroy_threshold_type = uint16_t;
      /**
       * \brief Number of allocator size bins.
      **/
      static constexpr const size_t c_bins = 18;
      /**
       * \brief Constructor.
      * @param allocator Global allocator for slabs.
      **/
      thread_allocator_t(global_allocator &allocator);
      thread_allocator_t(thread_allocator_t<global_allocator, allocator, allocator_traits> &) = delete;
      thread_allocator_t(thread_allocator_t<global_allocator, allocator, allocator_traits> &&ta) = default;
      thread_allocator_t &operator=(const thread_allocator_t<global_allocator, allocator, allocator_traits> &) = delete;
      thread_allocator_t &operator=(thread_allocator_t<global_allocator, allocator, allocator_traits> &&) = default;
      ~thread_allocator_t();
      /**
       * \brief Return the bin id for a given size.
      **/
      static size_t find_block_set_id(size_t sz);
      /**
       * \brief Fill mutiple array with reasonable default values.
      **/
      void fill_multiples_with_default_values();
      /**
       * \brief Set the allocation multiple for a given allocator bin.
      **/
      bool set_allocator_multiple(size_t id, size_t multiple);
      /**
       * \brief Return the allocation multiple for a given allocator bin.
      **/
      size_t get_allocator_multiple(size_t id) noexcept;
      /**
       * \brief Return the block size for a given allocator bin.
      **/
      size_t get_allocator_block_size(size_t id) const noexcept;
      /**
       * \brief Return a reference to the allocator block set for a given size.
      **/
      this_allocator_block_set_t &allocator_by_size(size_t sz) noexcept;
      /**
       * \brief Return an array of block sizes for all of the bins.
      **/
      ::std::array<size_t, c_bins> allocator_block_sizes() const noexcept;
      /**
      * \brief Allocate memory of size.
      *
      * @return nullptr on error.
      **/
      void *allocate(size_t size) NO_THREAD_SAFETY_ANALYSIS;
      /**
      * \brief Destroy a pointer allocated by this allocator.
      *
      * The common reason for failure is if this allocator did not make the pointer.
      * @return True on success, false on failure.
      **/
      bool destroy(void *v);
      /**
       * \brief  Return the array of multiples for debugging purposes.
      **/
      auto allocator_multiples() const -> const ::std::array<size_t, c_bins> &;
      /**
       * \brief Return the array of allocators for debugging purposes.
      **/
      auto allocators() const -> const ::std::array<this_allocator_block_set_t, c_bins> &;
      /**
       * \brief Free all empty blocks back to allocator.
       *
       * @param min_to_leave Minimum number of free blocks to leave in this set.
       * @param force True if should force freeing even if suboptimal timing.
      **/
      void free_empty_blocks(size_t min_to_leave, bool force);
      /**
       * \brief Return threshold destroy count for checking if should return memory to global.
       **/
      auto destroy_threshold() const noexcept -> destroy_threshold_type;
      /**
       * \brief Return the minimum number of local blocks.
       **/
      auto minimum_local_blocks() const noexcept -> uint16_t;
      /**
       * \brief Set threshold destroy count for checking if should return memory to global.
       **/
      void set_destroy_threshold(destroy_threshold_type threshold);
      /**
       * \brief Set minimum number of local blocks.
       **/
      void set_minimum_local_blocks(uint16_t minimum);

      /**
       * \brief Do maintance on thread associated blocks.
       *
       * This incldues coalescing, etc.
      **/
      void _do_maintenance();

    private:
      /**
       * \brief Global allocator used for getting slabs.
      **/
      global_allocator &m_allocator;
      /**
       * \brief Allocator multiple for requesting new blocks.
      **/
      ::std::array<thread_allocator_abs_data_t, c_bins> m_allocator_multiples;
      /**
       * \brief Allocators used to allocate various sizes of memory.
      **/
      ::std::array<this_allocator_block_set_t, c_bins> m_allocators;
      /**
       * \brief Threshold destroy count for checking if should return memory to global.
      **/
      destroy_threshold_type m_destroy_threshold = 100;
      /**
       * \brief Minimum local blocks.

       * This sets the minimum number of blocks left after returning memory to global.
      **/
      uint16_t m_minimum_local_blocks = 2;
    };
    /**
     * \brief Stream output for debugging.
    **/
    template <typename charT, typename Traits, typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    ::std::basic_ostream<charT, Traits> &operator<<(::std::basic_ostream<charT, Traits> &os,
                                                    const thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits> &ta);
  }
}
#include "allocator.hpp"
