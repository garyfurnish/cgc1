#pragma once
#include <mcppalloc/object_state.hpp>
#include "allocator_block_set.hpp"
#include "thread_allocator.hpp"
#include <mcppalloc/mcppalloc_utils/posix_slab.hpp>
#include <mcppalloc/mcppalloc_utils/win32_slab.hpp>
#include <mcppalloc/mcppalloc_utils/concurrency.hpp>
#include <mcppalloc/mcppalloc_utils/memory_range.hpp>
#include <map>
#include <mcppalloc/mcppalloc_utils/boost/container/flat_map.hpp>
#include "allocator_block_handle.hpp"
namespace mcppalloc
{
  namespace sparse
  {
    namespace details
    {
      /**
       * \brief Pair of memory addresses (begin and end)
       **/
      using memory_pair_t = typename ::std::pair<uint8_t *, uint8_t *>;
      /**
       * \brief Size comparator for memory pair.
       **/
      struct memory_pair_size_comparator_t {
        bool operator()(const memory_pair_t &a, const memory_pair_t &b) const noexcept
        {
          return system_memory_range_t(a).size() < system_memory_range_t(b).size();
        }
      };
      /**
       * \brief Global slab interval allocator, used by thread allocators.
       *
       * The intent of this allocator is to maximize linearity of cache accesses.
       * Each allocator has thread allocators which manage allocator block sets.
       * There is a allocator block set for an interval of possible allocation sizes.
       * Each allocator block set contains allocator blocks stored linearly.
       *
       * The allocator stores a list of global blocks that have been returned from thread allocators.
       * These can later be reused by other threads.
       * The allocator stores a free list of locations not used in the slab.
       * The free list must be occasionally collected to defragment the free list.
      **/
      template <typename Allocator_Policy = default_allocator_policy_t<::std::allocator<void>>>
      class allocator_t
      {
      public:
        using value_type = uint8_t;
        using reference = uint8_t &;
        using const_reference = const uint8_t &;
        using pointer = uint8_t *;
        using const_pointer = const uint8_t *;
        using iterator = uint8_t *;
        using const_iterator = const uint8_t *;
        using difference_type = ::std::ptrdiff_t;
        using size_type = size_t;
        using mutex_type = mutex_t;
        using allocator_policy_type = Allocator_Policy;
        using this_type = allocator_t<allocator_policy_type>;
        using block_type = block_t<allocator_policy_type>;

        using allocator_thread_policy_type = typename allocator_policy_type::thread_policy_type;
        /**
         * \brief Allocator used internally by this allocator for control structures.
        **/
        using allocator = typename allocator_policy_type::internal_allocator_type;
        /**
         * \brief Allocator block type for this allocator.
         *
         * This uses the control allocator for control memory.
         **/
        using allocator_block_type = allocator_block_t<allocator_policy_type>;
        /**
         * \brief Object state type for allocator blocks for this allocator.
         **/
        using object_state_type = typename allocator_block_type::object_state_type;
        /**
         * \brief Type that is a Vector of memory addresses.
         *
         * This uses the control allocator for control memory.
        **/
        using memory_pair_vector_t =
            typename ::std::vector<system_memory_range_t, typename allocator::template rebind<system_memory_range_t>::other>;
        /**
         * \brief Type of thread allocator used by this allocator.
         *
         * This uses the control allocator for control memory.
        **/
        using this_thread_allocator_t = thread_allocator_t<this_type, allocator_policy_type>;
        using thread_allocator_type = this_thread_allocator_t;
        /**
         * \brief Type of handles to blocks in this allocator.
        **/
        using this_allocator_block_handle_t = allocator_block_handle_t<this_type>;
        static_assert(::std::is_pod<this_allocator_block_handle_t>::value, "");
        /**
         * \brief Constructor.
         **/
        allocator_t();
        allocator_t(const allocator_t &) = delete;
        /**
         * \brief Move constructor.
         **/
        allocator_t(allocator_t &&) = default;
        allocator_t &operator=(const allocator_t &) = delete;
        /**
         * \brief Move assignment.
         **/
        allocator_t &operator=(allocator_t &&) = default;
        /**
         * \brief Destructor.
         **/
        ~allocator_t();
        void shutdown() REQUIRES(!m_mutex);
        /**
         * \brief Initialize the allocator.
         *
         * Note that suggested max heap size does not guarentee the heap can expand to that size depending on platform.
         * @param initial_gc_heap_size Initial size of gc heap.
         * @param max_heap_size Hint about how large the gc heap may grow.
         * @return True on success, false on failure.
        **/
        bool initialize(size_t initial_gc_heap_size, size_t max_heap_size) REQUIRES(!m_mutex);
        /**
         * \brief Return a thread allocator for the current thread.
         *
         * If the thread is not initialized, initialize it.
         **/
        this_thread_allocator_t &initialize_thread() REQUIRES(!m_mutex);
        /**
         * \brief Destroy thread local data for currently running thread.
        **/
        void destroy_thread() REQUIRES(!m_mutex);
        /**
         * \brief Get an interval of memory.
         *
         * This does not throw an exception on error but instead returns a special value.
         * @param try_expand Attempt to expand underlying slab if necessary
         * @return (nullptr,nullptr) on error.
        **/
        system_memory_range_t get_memory(size_t sz, bool try_expand) REQUIRES(!m_mutex);

        /**
         * \brief Get an interval of memory.
         *
         * Requires holding lock.
         * This does not throw an exception on error but instead returns a special value.
         * @param try_expand Try to expand underlying slab if true.
         * @return (nullptr,nullptr) on error.
        **/
        system_memory_range_t _u_get_memory(size_t sz, bool try_expand) REQUIRES(m_mutex);
        /**
         * \brief Create or reuse an allocator block in destination by reference.
         *
         * The allocation block returned must be registered.
         * @param ta Thread allocator requesting block.
         * @param create_sz Size of block requested.
         * @param minimum_alloc_length Minimum allocation length for block.
         * @param maximum_alloc_length Maximum allocation length for block.
         * @param destination Returned allocator block.
         * @param try_expand Attempt to expand underlying slab if necessary
         * @return True on success, false on failure.
        **/
        bool _u_get_unregistered_allocator_block(this_thread_allocator_t &ta,
                                                 size_t create_sz,
                                                 size_t minimum_alloc_length,
                                                 size_t maximum_alloc_length,
                                                 size_t allocate_size,
                                                 allocator_block_type &destination,
                                                 bool try_expand) REQUIRES(m_mutex);

        /**
         * \brief Release an interval of memory.
         *
         * @param pair Memory interval to release.
        **/
        void release_memory(const memory_pair_t &pair) REQUIRES(!m_mutex);
        /**
         * \brief Release an interval of memory.
         *
         * Requires holding lock.
         * @param pair Memory interval to release.
        **/
        void _u_release_memory(const memory_pair_t &pair) REQUIRES(m_mutex);

        /**
         * \brief Return true if the interval of memory is in the free list.
         *
         * @param pair Memory interval to test.
         **/
        REQUIRES(!m_mutex) auto in_free_list(const memory_pair_t &pair) const noexcept -> bool;
        /**
         * \brief Return length of free list.
         **/
        REQUIRES(!m_mutex) auto free_list_length() const noexcept -> size_t;
        /**
         * \brief Instead of destroying a block that is still in use, release to global allocator control.
         *
         * @param block Block to release.
        **/
        void to_global_allocator_block(allocator_block_type &&block) REQUIRES(!m_mutex);
        /**
         * \brief Destroy an allocator block.
         *
         * @param ta Requesting thread allocator.
         * @param block Block to destroy.
        **/
        void destroy_allocator_block(this_thread_allocator_t &ta, allocator_block_type &&block) REQUIRES(!m_mutex);
        /**
         * \brief Destroy an allocator block not owned by a thread.
         *
         * Requires holding lock.
         * @param block Block to destroy.
        **/
        void _u_destroy_global_allocator_block(allocator_block_type &&block) REQUIRES(m_mutex);

        /**
         * \brief Register a allocator block before moving/destruction.
         *
         * @param ta Requesting thread allocator.
         * @param block Block to request registartion of.
        **/
        //      void register_allocator_block(this_thread_allocator_t &ta, allocator_block_type &block) REQUIRES(!m_mutex);
        /**
         * \brief Register a allocator block before moving/destruction.
         *
         * Requires holding lock.
         * @param ta Requesting thread allocator.
         * @param block Block to request registartion of.
        **/
        //      void _u_register_allocator_block(this_thread_allocator_t &ta, allocator_block_type &block) REQUIRES(m_mutex);
        /**
         * \brief Unregister a registered allocator block before moving/destruction.
         *
         * @param block Block to request unregistration of.
        **/
        void unregister_allocator_block(allocator_block_type &block) REQUIRES(!m_mutex);

        /**
         * \brief Unregister a registered allocator block before moving/destruction.
         *
         * Requires holding lock
         * @param block Block to request unregistration of.
        **/
        void _u_unregister_allocator_block(allocator_block_type &block) REQUIRES(m_mutex);

        /**
         * \brief Move registered allocator blocks by iteration.
         *
         * The right way to think about this as moving a container to a new memory address.
         * Requires holding lock.
         * @param begin Start iterator.
         * @param end End iterator.
         * @param offset New container is offset from old container (so positive for old is at a smaller memory address).
        **/
        template <typename Iterator>
        void _u_move_registered_blocks(const Iterator &begin, const Iterator &end, ptrdiff_t offset) REQUIRES(m_mutex);

        /**
         * \brief Move registered allocator blocks in container by offset.
         *
         * Requires holding lock.
         * @param blocks New container reference.
         * @param offset New container is offset from old container (so positive for old is at a smaller memory address).
        **/
        template <typename Container>
        void _u_move_registered_blocks(Container &blocks, ptrdiff_t offset) REQUIRES(m_mutex);

        /**
         * \brief Move registered allocator block.
         *
         * Requires holding lock.
         * The allocator has references to where allocator blocks are stored.
         * Therefore if the block is moved, the allocator must be informed.
         * @param old_block Address of old allocator block location.
         * @param new_block Address of new allocator block location.
         **/
        void _u_move_registered_block(allocator_block_type *old_block, allocator_block_type *new_block) REQUIRES(m_mutex);
        /**
         * \brief Find the block for this allocated memory address to find.
         *
         * Requires holding lock.
         * Note that another thread could move the allocator block.
         * However, this can't happen without registering another block.
         * Therefore, the handle is only valid during this lock.
         * It is important to note this does not throw on error.  Instead it returns nullptr.
         * @param addr Address of allocated memory to find.
         * @return nullptr on error.
        **/
        const this_allocator_block_handle_t *_u_find_block(void *addr) REQUIRES(m_mutex);
        /**
         * \brief Return the beginning of the underlying slab.
        **/
        uint8_t *begin() const REQUIRES(!m_mutex);
        /**
         * \brief Return the end of the underlying slab.
        **/
        uint8_t *end() const REQUIRES(!m_mutex);
        /**
         * \brief Return the end of the currently used portion of the slab.
        **/
        uint8_t *current_end() const REQUIRES(!m_mutex);
        /**
         * \brief Return the size of the slab.
         **/
        REQUIRES(!m_mutex) auto size() const noexcept -> size_t;
        /**
         * \brief Return the currently used size of slab.
         **/
        REQUIRES(!m_mutex) auto current_size() const noexcept -> size_t;
        /**
         * \brief Return the size of the slab.
         **/
        REQUIRES(m_mutex) auto _u_size() const noexcept -> size_t;
        /**
         * \brief Return the currently used size of slab.
         **/
        REQUIRES(m_mutex) auto _u_current_size() const noexcept -> size_t;
        /**
         * \brief Return maximum heap size.
         **/
        auto max_heap_size() const noexcept -> size_type;
        /**
         * \brief Collapse the free list.
        **/
        void collapse() REQUIRES(!m_mutex);
        /**
         * \brief Return a reference to the spinlock.
         **/
        mutex_type &_mutex() RETURN_CAPABILITY(m_mutex)
        {
          return m_mutex;
        }
        /**
         * \brief Destroy all memory pairs in the container.
         *
         * This clears the container when done.
         **/
        template <typename Container>
        void bulk_destroy_memory(Container &container) REQUIRES(!m_mutex);
        /**
         * \brief Return the free list for debugging purposes.
        **/
        memory_pair_vector_t _d_free_list() const REQUIRES(!m_mutex);
        /**
         * \brief Return the free list for debugging purposes without locking.
        **/
        const memory_pair_vector_t &_ud_free_list() const REQUIRES(m_mutex);
        /**
         * \brief Return the beginning of the underlying slab.
         **/
        uint8_t *_u_begin() const noexcept;
        /**
         * \brief Return the end of the underlying slab.
        **/
        uint8_t *_u_end() const noexcept;
        /**
         * \brief Return the end of the currently used portion of the slab.
         *
         * Requires holding lock.
        **/
        uint8_t *_u_current_end() const REQUIRES(m_mutex);
        /**
         * \brief Internal consistency checks without locking.
        **/
        void _ud_verify() REQUIRES(m_mutex);
        /**
         * \brief Perform internal consistency checks.
        **/
        void _d_verify() REQUIRES(!m_mutex);
        /**
         * \brief Return true if the destructor has been called.
         **/
        bool is_shutdown() const;
        /**
         * \brief Return reference to thread alloctator policy.
         **/
        auto thread_policy() noexcept -> allocator_thread_policy_type &;
        /**
         * \brief Return reference to allocator traits.
         **/
        auto thread_policy() const noexcept -> const allocator_thread_policy_type &;
        /**
         * \brief Return number of global blocks.
        **/
        size_t num_global_blocks() REQUIRES(!m_mutex);
        /**
         * \brief Return number of global blocks without locking.
         *
         * Requires holding lock.
        **/
        size_t _u_num_global_blocks() REQUIRES(m_mutex);

        /**
         * \brief Collect/Coalesce global blocks.
         **/
        void collect() REQUIRES(!m_mutex);
        /**
         * \brief Collect/Coalesce global blocks.
         *
         * Requires holding lock.
         **/
        void _u_collect() REQUIRES(m_mutex);

        /**
         * \brief Register a allocator block before moving/destruction.
         *
         * Requires holding lock.
         * @param ta Requesting thread allocator.
         * @param block Block to register.
        **/
        void _u_register_allocator_block(this_thread_allocator_t &ta, allocator_block_type &block) REQUIRES(m_mutex);
        /**
         * \brief Put information about allocator into a property tree.
         * @param level Level of information to give.  Higher is more verbose.
         **/
        void to_ptree(::boost::property_tree::ptree &ptree, int level) const REQUIRES(!m_mutex);
        /**
         * \brief Set all threads to force free empty blocks.
         * Typically called after state is externally altered from gc.
         **/
        void _u_set_force_free_empty_blocks() REQUIRES(m_mutex);

      private:
        /**
         * \brief Vector type for storing blocks held by the global allocator.
         **/
        using global_block_vector_type = rebind_vector_t<allocator_block_type, allocator>;
        /**
         * \brief Return an allocator block.
         *
         * This function does actual creation of block without any sort of registartion.
         * Requires holding lock.
         * @param sz Size of block requested.
         * @param minimum_alloc_length Minimum allocation length for block.
         * @param maximum_alloc_length Maximum allocation length for block.
         * @param block Return block by reference.
         * @param try_expand Attempt to expand underlying slab if necessary
        **/
        REQUIRES(m_mutex)
        bool _u_create_allocator_block(this_thread_allocator_t &ta,
                                       size_t sz,
                                       size_t minimum_alloc_length,
                                       size_t maximum_alloc_length,
                                       allocator_block_type &block,
                                       bool try_expand);
        /**
         * \brief Find a global allocator block that has sz free for allocation.
         *
         * The returned block will have compatible min and max lengths.
         * @param sz Size of memory available for allocation.
         * @param minimum_alloc_length Minimum allocation length for block.
         * @param maximum_alloc_length Maximum allocation length for block.
         **/
        REQUIRES(m_mutex)
        auto _u_find_global_allocator_block(size_t sz, size_t minimum_alloc_length, size_t maximum_alloc_length) ->
            typename global_block_vector_type::iterator;

        /**
         * \brief Internal helper function for moving registered blocks.
         *
         * This function moves existing blocks in m_blocks to a new appropriate location.
         * This saves the number of searches required from O(num blocks) to O(sets of contiguous blocks).
         * @param num Number of contiguous blocks.
         * @param new_location Beginning of new location of blocks on outside.
         * @param lb Lower bound where the blocks are currently in m_blocks.
         **/
        template <typename Iterator, typename LB>
        void _u_move_registered_blocks_contiguous(size_t num, const Iterator &new_location, const LB &lb) REQUIRES(m_mutex);

        /**
         * \brief Unregister a registered allocator block before moving/destruction without locking.
         *
         * @param ta Requesting thread allocator.
         * @param block Block to unregister.
        **/
        void _u_unregister_allocator_block(this_thread_allocator_t &ta, allocator_block_type &block) REQUIRES(m_mutex);
        /**
         * \brief Mutex for this allocator.
         **/
        mutable mutex_type m_mutex;
        /**
         * \brief This is set during destruction.
         *
         * Certain features of the allocator need to behave differently during destruction.
        **/
        std::atomic<bool> m_shutdown{false};
        /**
         * \brief Free interval vector.
         *
         * List of all intervals of free memory in the slab.
         * This is not stored in sorted order.
         * This must be collapsed occasionally.
        **/
        memory_pair_vector_t m_free_list GUARDED_BY(m_mutex);
        /**
         * \brief Pointer to end of currently used portion of slab.
        **/
        uint8_t *m_current_end GUARDED_BY(m_mutex) = nullptr;
        /**
         * \brief Initial size of gc heap.
        **/
        size_t m_initial_gc_heap_size GUARDED_BY(m_mutex) = 0;
        /**
         * \brief Minimum size to expand heap by.
        **/
        size_t m_minimum_expansion_size GUARDED_BY(m_mutex) = 0;
        /**
         * \brief Underlying slab.
        **/
        slab_t m_slab;
        /**
         * \brief Type that is an owning pointer to a thread allocator that uses the control allocator to handle memory.
         **/
        using thread_allocator_unique_ptr_t =
            typename ::std::unique_ptr<this_thread_allocator_t,
                                       typename cgc_allocator_deleter_t<this_thread_allocator_t, allocator>::type>;
        /**
         * \brief Type that is a map for thread allocators that uses the control allocator to handle memory.
         **/
        using ta_map_allocator_t = typename allocator::template rebind<
            typename ::std::pair<typename ::std::thread::id, thread_allocator_unique_ptr_t>>::other;
        /**
         * \brief Blocks currently in use.
         *
         * This is sorted by the address of the beginning of the block.
        **/
        rebind_vector_t<this_allocator_block_handle_t, allocator> m_blocks;
        /**
         * \brief Vector type for storing blocks held by the global allocator.
         *
         * This holds blocks that have active memory in them that have been returned by thread allocators.
         * The most common reason for this is that the thread alllocators have terminated.
         * This uses the control allocator for control memory.
         **/
        global_block_vector_type m_global_blocks;
        /**
         * \brief Map from thread ids to thread allocators.
        **/
        ::boost::container::
            flat_map<::std::thread::id, thread_allocator_unique_ptr_t, ::std::less<::std::thread::id>, ta_map_allocator_t>
                m_thread_allocators GUARDED_BY(m_mutex);

        static_assert(::std::is_base_of<allocator_policy_tag_t, allocator_policy_type>::value, "");
        static_assert(::std::is_base_of<::mcppalloc::details::allocator_thread_policy_tag_t,
                                        typename allocator_policy_type::thread_policy_type>::value,
                      "");
        /**
         * \brief Thread allocator policy.
        **/
        typename allocator_policy_type::thread_policy_type m_thread_policy;

        /**
         * \brief Maximum heap size.
         **/
        size_type m_maximum_heap_size = 0;

      public:
        /**
         * \brief Internal function to return blocks currently in use.
         *
         * Requires holding lock.
        **/
        auto _u_blocks() -> decltype(m_blocks) &;
        /**
         * \brief Internal debug function to return blocks owned by allocator.
         **/
        auto _ud_global_blocks() -> global_block_vector_type &;
      };
    }
    template <typename Allocator_Policy = default_allocator_policy_t<::std::allocator<void>>>
    using allocator_t = details::allocator_t<Allocator_Policy>;
  }
}
#include "thread_allocator_impl.hpp"
#include "allocator_impl.hpp"
