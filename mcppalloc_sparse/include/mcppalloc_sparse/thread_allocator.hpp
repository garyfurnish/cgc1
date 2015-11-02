#pragma once
#include <mcppalloc_utils/concurrency.hpp>
#include "object_state.hpp"
#include "allocator_block_set.hpp"
#include "thread_allocator_abs_data.hpp"
#include <array>
#include <boost/property_tree/ptree_fwd.hpp>
#include <mcppalloc_utils/memory_range.hpp>
namespace mcppalloc
{
  namespace sparse
  {
    namespace details
    {
      /**
       * \brief Output the memory pair as a stirng to the stream.
      **/
      template <typename T>
      void print_memory_pair(T &os, const ::std::pair<uint8_t *, uint8_t *> &pair);
      /**
       * \brief Output the memory pair to the string.
      **/
      inline ::std::string to_string(const ::std::pair<uint8_t *, uint8_t *> &pair);
      template <typename Allocator_Policy>
      class allocator_t;
      /**
       * \brief Per thread allocator for a global allocator.
       * @tparam Global_Allocator Global Allocator that owns this thread allocator.
      **/
      template <typename Global_Allocator, typename Allocator_Policy>
      class thread_allocator_t
      {
      public:
        using allocator_policy_type = Allocator_Policy;
        /**
         * \brief Type of allocator to use for internal operations.
        **/
        using allocator = typename allocator_policy_type::internal_allocator_type;
        /**
         * \brief Type of global allocator that created this thread allocator.
        **/
        using global_allocator = Global_Allocator;
        /**
         * \brief Allocator traits for global allocator.
        **/
        using allocator_traits = typename allocator_policy_type::thread_policy_type;
        using size_type = typename allocator_policy_type::size_type;
        /**
         * \brief Allocator block set for this thread allocator.
        **/
        using this_allocator_block_set_t = allocator_block_set_t<Allocator_Policy>;
        using this_block_type = typename this_allocator_block_set_t::allocator_block_type;
        /**
         * \brief Type for destroy threshold.
         *
         * This is here because maybe in the future someone wants to support uint32_t.
         * However, uint16_t is better for cache locality.
        **/
        using destroy_threshold_type = uint32_t;
        /**
         * \brief Number of allocator size bins.
        **/
        static constexpr const size_t c_bins = 18;
        /**
         * \brief Constructor.
        * @param allocator Global allocator for slabs.
        **/
        thread_allocator_t(global_allocator &allocator);
        thread_allocator_t(thread_allocator_t &) = delete;
        thread_allocator_t(thread_allocator_t &&ta) = default;
        thread_allocator_t &operator=(const thread_allocator_t &) = delete;
        thread_allocator_t &operator=(thread_allocator_t &&) = default;
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
        void *allocate(size_t size);
        /**
         * \brief Attempt to allocate once.
         *
         * @return nullptr on error.
         **/
        void *_allocate_once(size_t size);
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
        auto allocator_multiples() const -> const ::std::array<thread_allocator_abs_data_t, c_bins> &;
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
         * \brief Force a destruction of empty blocks.
        **/
        void set_force_free_empty_blocks() noexcept;
        /**
         * \brief Do maintance on thread associated blocks.
         *
         * This incldues coalescing, etc.
        **/
        void _do_maintenance();
        /**
         * \brief Return the bytes of primary memory used.
         **/
        auto primary_memory_used() const noexcept -> size_type;
        /**
         * \brief Return the bytes of secondary memory used.
         **/
        auto secondary_memory_used() const noexcept -> size_type;
        /**
         * \brief Return the bytes of secondary memory used.
         **/
        auto secondary_memory_used_self() const noexcept -> size_type;
        /**
         * \brief Shrink secondary data structures to fit.
         **/
        void shrink_secondary_memory_usage_to_fit();
        /**
         * \brief Shrink secondary data structures to fit for self only.
         **/
        void shrink_secondary_memory_usage_to_fit_self();

        /**
         * \brief Put information about thread allocator into a property tree.
         * @param level Level of information to give.  Higher is more verbose.
         **/
        void to_ptree(::boost::property_tree::ptree &ptree, int level) const;
        /**
         * \brief Put information about thread allocator into a json string.
         * @param level Level of information to give.  Higher is more verbose.
         **/
        auto to_json(int level) const -> ::std::string;

      private:
        /**
         * \brief Free empty blocks, but only if necessary.
         **/
        void _check_do_free_empty_blocks();
        /**
         * \brief Free empty blocks, but only if necessary.
         * @param allocator Allocator to condition on.
         **/
        void _check_do_free_empty_blocks(this_allocator_block_set_t &allocator);

        /**
         * \brief Execute free empty blocks.
         **/
        void _do_free_empty_blocks();
        /**
         * \brief Attempt to add an allocator block with a given id.
         * @param id Id to try to add.
         * @param sz Request size.
         * @param try_expand Attempt to expand underlying slab if necessary
         * @return True on success, false on failure.
         **/
        bool _add_allocator_block(size_t id, size_t sz, bool try_expand);
        /**
         * \brief Allocators used to allocate various sizes of memory.
        **/
        ::std::array<this_allocator_block_set_t, c_bins> m_allocators;
        /**
         * \brief Global allocator used for getting slabs.
        **/
        global_allocator &m_allocator;
        /**
         * \brief Threshold destroy count for checking if should return memory to global.
        **/
        uint16_t m_destroy_threshold = ::std::numeric_limits<uint16_t>::max();
        /**
         * \brief Set this to true to force destruction of empty blocks.
         *
         * For instance a garbage collector may want to use this.
         * This lets you turn the destroy threshold very high.
         **/
        ::std::atomic<bool> m_force_free_empty_blocks{false};
        /**
         * \brief Allocator multiple for requesting new blocks.
        **/
        ::std::array<thread_allocator_abs_data_t, c_bins> m_allocator_multiples;
        /**
         * \brief Minimum local blocks.

         * This sets the minimum number of blocks left after returning memory to global.
        **/
        uint16_t m_minimum_local_blocks = 2;
      };
      /**
       * \brief Stream output for debugging.
      **/
      template <typename charT, typename Traits, typename Global_Allocator, typename Allocator_Policy>
      ::std::basic_ostream<charT, Traits> &operator<<(::std::basic_ostream<charT, Traits> &os,
                                                      const thread_allocator_t<Global_Allocator, Allocator_Policy> &ta);
    }
  }
}
#include "allocator.hpp"
