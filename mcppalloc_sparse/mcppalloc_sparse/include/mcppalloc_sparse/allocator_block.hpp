#pragma once
#include <vector>
#include <algorithm>
#include <string>
#include <mcppalloc_utils/warning_wrapper_push.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <mcppalloc_utils/warning_wrapper_pop.hpp>
#include "allocator_block.hpp"
#include <mcppalloc/block.hpp>
#include <mcppalloc/object_state.hpp>
#include <mcppalloc_utils/make_unique.hpp>
#include <mcppalloc_utils/container.hpp>
#include <mcppalloc/default_allocator_policy.hpp>
namespace mcppalloc
{
  namespace sparse
  {
    namespace details
    {
      /**
       * \brief Allocator block.
       *
       * This is designed to be moved to where it is used for cache linearity.
       * The fields in this should be organized to maximize cache hits.
       * This class is incredibly cache layout sensitive.
       *
       * Internally this implements a linked list of object_state_t followed by data.
       * The last valid object_state_t always points to an object_state_t at end().
       **/
      template <typename Allocator_Policy>
      class allocator_block_t
      {
      public:
        using size_type = size_t;
        using allocator_policy_type = Allocator_Policy;
        static_assert(::std::is_base_of<allocator_policy_tag_t, allocator_policy_type>::value,
                      "Allocator policy must be allocator_policy");
        using allocator = typename allocator_policy_type::internal_allocator_type;
        using user_data_type = typename allocator_policy_type::user_data_type;
        using object_state_type = ::mcppalloc::details::object_state_t<allocator_policy_type>;
        static user_data_type s_default_user_data;
        using block_type = block_t<allocator_policy_type>;
        static constexpr size_type minimum_header_alignment() noexcept
        {
          return allocator_policy_type::cs_minimum_alignment;
        }

        allocator_block_t() = default;
        /**
         * \brief Constructor
         * @param start Start of memory block that this allocator uses.
         * @param length Length of memory block that this allocator uses
         * @param minimum_alloc_length Minimum length of object that can be allocated using this allocator.
         * @param maximum_alloc_length Maximum length of object that can be allocated using this allocator.
        **/
        CGC1_ALWAYS_INLINE
        allocator_block_t(void *start, size_t length, size_t minimum_alloc_length, size_t maximum_alloc_length) noexcept;
        allocator_block_t(const allocator_block_t &) = delete;
        CGC1_ALWAYS_INLINE allocator_block_t(allocator_block_t &&) noexcept;
        allocator_block_t &operator=(const allocator_block_t &) = delete;
        CGC1_ALWAYS_INLINE allocator_block_t &operator=(allocator_block_t &&) noexcept;
        ~allocator_block_t();
        /**
         * \brief Return false if items are allocated.  Otherwise may return true or false.
         *
         * Return true if no items allocated.
         * Return false if items are allocated or if no items are allocated and a collection is needed.
         * Note the semantics here!
        **/
        bool empty() const noexcept;
        /**
         * \brief Return true if no more allocations can be performed.
        **/
        bool full() const noexcept;
        /**
         * \brief Begin iterator to allow bytewise iteration over memory block.
        **/
        uint8_t *begin() const noexcept;
        /**
         * \brief End iterator to allow bytewise iteration over memory block.
        **/
        uint8_t *end() const noexcept;
        /**
         * \brief Return size of memory.
         **/
        auto memory_size() const noexcept -> size_type;
        /**
         * \brief End iterator for object_states
        **/
        auto current_end() const noexcept -> object_state_type *;
        /**
         * \brief Return beginning object state.
         *
         * Produces undefined behavior if no valid object states.
        **/
        auto _object_state_begin() const noexcept -> object_state_type *;
        /**
         * \brief Find the object state associated with the given address.
         *
         * @return Associated object state, nullptr if not found.
        **/
        auto find_address(void *addr) const noexcept -> object_state_type *;
        /**
         * \brief Allocate size bytes on the block.
         *
         * @return Valid pointer if possible, nullptr otherwise.
        **/
        auto allocate(size_t size) -> block_type;
        /**
         * \brief Destroy a v that is on the block.
         *
         * The usual cause of failure would be the pointer not being in the block.
         * @return True on success, false on failure.
        **/
        bool destroy(void *v);
        /**
         * \brief Destroy a v that is on the block.
         *
         * The usual cause of failure would be the pointer not being in the block.
         * @param v Pointer to destroy.
         * @param last_collapsed_size Max size of allocation made available by destroying.
         * @param last_max_alloc_available Return the previous last max alloc available.
         * @return True on success, false on failure.
        **/
        bool destroy(void *v, size_t &last_collapsed_size, size_t &last_max_alloc_available);
        /**
         * \brief Collect any adjacent blocks that may have formed into one block.
         * @param num_quasifreed Increment by number of quasifreed found.
        **/
        void collect(size_t &num_quasifreed);
        /**
         * \brief Return the maximum allocation size available.
        **/
        size_t max_alloc_available();
        /**
         * \brief Verify object state os.
         *
         * Code may optimize out in release mode.
         * @param os Object state to verify.
         **/
        void _verify(const object_state_type *os);
        /**
         * \brief Return true if valid, false otherwise.
         **/
        bool valid() const noexcept;
        /**
         * \brief Clear all control structures and invalidate.
         **/
        void clear();
        /**
         * \brief Return minimum object allocation length.
        **/
        size_t minimum_allocation_length() const;
        /**
         * \brief Return maximum object allocation length.
        **/
        size_t maximum_allocation_length() const;
        /**
         * \brief Return updated last max alloc available.
         **/
        size_t last_max_alloc_available() const noexcept;
        /**
         * \brief Return the bytes of secondary memory used.
         **/
        size_t secondary_memory_used() const noexcept;
        /**
         * \brief Shrink secondary data structures to fit.
         **/
        void shrink_secondary_memory_usage_to_fit();
        /**
         * \brief Put information about abs into a property tree.
         * @param level Level of information to give.  Higher is more verbose.
         **/
        void to_ptree(::boost::property_tree::ptree &ptree, int level) const;

      public:
        /**
         * \brief Free list for this block.
         *
         * Uses control allocator for control data.
         **/
        ::boost::container::flat_set<object_state_type *,
                                     ::mcppalloc::details::os_size_compare,
                                     typename allocator::template rebind<object_state_type *>::other> m_free_list;
        /**
         * \brief Next allocator pointer if whole block has not yet been used.
        **/
        object_state_type *m_next_alloc_ptr;
        /**
         * \brief End of memory block.
        **/
        uint8_t *m_end;
        /**
         * \brief Minimum object allocation length.
        **/
        size_t m_minimum_alloc_length;
        /**
         * \brief start of memory block.
        **/
        uint8_t *m_start;
        /**
         * \brief Default user data option.
         *
         * The allocated memory is stored in control allocator.
         **/
        allocator_unique_ptr_t<user_data_type, allocator> m_default_user_data;
        /**
         * \brief Updated last max alloc available.
         *
         * Designed to sync with allocator block sets.
         **/
        size_t m_last_max_alloc_available = 0;
        /**
         * \brief Maximum object allocation length.
         *
         * This is at end as it is not needed for allocation (cache allignment).
         **/
        size_t m_maximum_alloc_length = 0;
      };
    }
  }
}
#include "allocator_block_impl.hpp"
