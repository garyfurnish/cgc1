#pragma once
#include "internal_declarations.hpp"
#include <vector>
#include <algorithm>
#include <string>
#include <cgc1/warning_wrapper_push.hpp>
#include <boost/container/flat_set.hpp>
#include <cgc1/warning_wrapper_pop.hpp>
#include "allocator_block.hpp"
#include "object_state.hpp"
namespace cgc1
{
  template <typename T, typename allocator>
  using rebind_vector_t = typename ::std::vector<T, typename allocator::template rebind<T>::other>;

  namespace details
  {
    // type for representing infinite length.
    static constexpr const size_t c_infinite_length = static_cast<size_t>(-1);
    /**
     * \brief Allocator block.
     *
     * This is designed to be moved to where it is used for cache linearity.
     * The fields in this should be organized to maximize cache hits.
     * This class is incredibly cache layout sensitive.
     *
     * Internally this implements a linked list of object_state_t followed by data.
     * The last valid object_state_t always points to an object_state_t at end().
     * @tparam Allocator allocator to use for control data.
     * @tparam User_Data user data control.
     **/
    template <typename Allocator, typename User_Data = user_data_base_t>
    class allocator_block_t
    {
    public:
      using allocator = Allocator;
      using user_data_type = User_Data;
      using size_type = size_t;
      static user_data_type s_default_user_data;
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
      object_state_t *current_end() const noexcept;
      /**
       * \brief Return beginning object state.
       *
       * Produces undefined behavior if no valid object states.
      **/
      object_state_t *_object_state_begin() const noexcept;
      /**
       * \brief Find the object state associated with the given address.
       *
       * @return Associated object state, nullptr if not found.
      **/
      object_state_t *find_address(void *addr) const noexcept;
      /**
       * \brief Allocate size bytes on the block.
       *
       * @return Valid pointer if possible, nullptr otherwise.
      **/
      void *allocate(size_t size);
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
      void _verify(const object_state_t *os);
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

    public:
      /**
       * \brief Free list for this block.
       *
       * Uses control allocator for control data.
       **/
      ::boost::container::flat_set<object_state_t *,
                                   os_size_compare,
                                   typename allocator::template rebind<object_state_t *>::other> m_free_list;
      /**
       * \brief Next allocator pointer if whole block has not yet been used.
      **/
      object_state_t *m_next_alloc_ptr;
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
      unique_ptr_allocated<user_data_type, Allocator> m_default_user_data;
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
    /**
     * \brief Base class for user data associated with a entry.
     **/
    class user_data_base_t
    {
    public:
      user_data_base_t() = default;
      user_data_base_t(const user_data_base_t &) = default;
      user_data_base_t(user_data_base_t &&) = default;

    private:
      /**
       * \brief Magic constant that is used to check if this is user data.
       **/
      static constexpr size_t c_magic_constant = 0x8e6866a1;
      /**
       * \brief Store a magic constant.
       *
       * If this magic constant is present, this is probabilistically user data.
       **/
      size_t m_magic_constant = c_magic_constant;

    public:
      /**
       * \brief Return true if magic constant is valid.
       **/
      bool is_magic_constant_valid() const
      {
        return m_magic_constant == c_magic_constant;
      }
      /**
       * \brief Return beginning of allocator block.
       **/
      /*      uint8_t *allocator_block_begin() const
      {
        return m_allocator_block_begin;
        }*/
      /**
       * \brief Return beginning of allocator block as object state.
       **/
      /*      object_state_t *allocator_block_state_begin() const
      {
        return reinterpret_cast<object_state_t *>(allocator_block_begin());
        }*/

    private:
      /**
       * \brief Beginning of allocator block data associated with this allocation.
       *
       * Note this is a pointer to the data, not the block itself, so it is move safe.
       **/
      //      uint8_t *m_allocator_block_begin = nullptr;

    public:
      /**
       * True if is default, false otherwise.
      **/
      bool m_is_default = false;
      /**
       * \brief True if uncollectable, false otherwise.
       *
       * Only used for gc_type.
      **/
      bool m_uncollectable = false;
    };
    /**
     * \brief User data that can be associated with an allocation.
     **/
    class gc_user_data_t : public user_data_base_t
    {
    public:
      /**
       * \brief Constructor
       *
       * @param block Block that this allocation belongs to.
       **/
      gc_user_data_t() = default;
      /**
       * \brief Optional finalizer function to run.
      **/
      ::std::function<void(void *)> m_finalizer = nullptr;

    private:
    };
    /**
     * \brief Return true if the object state is a valid object state, false otherwise.
     *
     * Uses magic user data detection.
     * @param state State to test for validity.
     * @param user_data_range_begin Beginning of the memory range that is a valid location for user data.
     * @param user_data_range_end End of the memory range that is a valid location for user data.
    **/
    inline bool
    is_valid_object_state(const object_state_t *state, const uint8_t *user_data_range_begin, const uint8_t *user_data_range_end);
  }
}
#include "allocator_block_impl.hpp"
