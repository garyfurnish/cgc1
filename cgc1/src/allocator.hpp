#pragma once
#include "object_state.hpp"
#include "allocator_block_set.hpp"
#include "thread_allocator.hpp"
#include <cgc1/posix_slab.hpp>
#include <cgc1/win32_slab.hpp>
#include <cgc1/concurrency.hpp>
#include <sstream>
#include <map>
#include "internal_allocator.hpp"
#include "allocator_block_handle.hpp"
namespace cgc1
{
  namespace details
  {
    struct do_nothing_t {
      template <typename... Args>
      void operator()(Args &&...)
      {
      }
    };
    struct allocator_no_traits_t {
      do_nothing_t on_create_allocator_block;
      do_nothing_t on_destroy_allocator_block;
      do_nothing_t on_creation;
      using allocator_block_user_data_type = user_data_base_t;
    };
    class out_of_memory_exception_t : public ::std::runtime_error
    {
    public:
      out_of_memory_exception_t() : ::std::runtime_error("Allocator out of available memory")
      {
      }
    };
    /**
    Global slab interval allocator, used by thread allocators.
    The free list must be occasionally collected to defragment the free list.
    **/
    template <typename Allocator = ::std::allocator<void>, typename Allocator_Traits = allocator_no_traits_t>
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
      /**
      Allocator used in place of ::std::allocator for containers in this allocator if desired.
      **/
      using allocator = Allocator;
      /**
      Traits used.
      **/
      using allocator_traits = Allocator_Traits;
      using block_type = allocator_block_t<allocator, typename allocator_traits::allocator_block_user_data_type>;
      /**
      Pair of memory addresses (begin and end)
      **/
      using memory_pair_t = typename ::std::pair<uint8_t *, uint8_t *>;
      /**
      Vector of memory addresses.
      **/
      using memory_pair_vector_t =
          typename ::std::vector<memory_pair_t, typename allocator::template rebind<memory_pair_t>::other>;
      /**
      Type of thread allocator used by this allocator.
      **/
      using this_thread_allocator_t = thread_allocator_t<allocator_t<Allocator, Allocator_Traits>, Allocator, Allocator_Traits>;
      /**
      Type of this allocator.
      **/
      using this_allocator_t = allocator_t<Allocator, Allocator_Traits>;
      /**
      Type of handles in this allocator.
      **/
      using this_allocator_block_handle_t = allocator_block_handle_t<this_allocator_t>;
      allocator_t();
      allocator_t(const allocator_t &) = delete;
      allocator_t(allocator_t &&) = default;
      allocator_t &operator=(const allocator_t &) = delete;
      allocator_t &operator=(allocator_t &&) = default;
      ~allocator_t();
      /**
       * Initialize the allocator.
       * @param initial_gc_heap_size Initial size of gc heap.
       * @param suggested_max_heap_size Hint about how large the gc heap may grow.
       * @return True on success, false on failure.
      **/
      bool initialize(size_t initial_gc_heap_size, size_t suggested_max_heap_size) REQUIRES(!m_mutex);
      this_thread_allocator_t &initialize_thread() REQUIRES(!m_mutex);
      /**
       * \brief Destroy thread local data for currently running thread.
      **/
      void destroy_thread() REQUIRES(!m_mutex);
      /**
       * \brief Get an interval of memory.
       *
       * This does not throw an exception on error but instead returns a special value.
       * @return (nullptr,nullptr) on error.
      **/
      memory_pair_t get_memory(size_t sz) REQUIRES(!m_mutex);
      /**
       * \brief Return an allocator block by reference.
       @param destination Returned allocator block.
      **/
      REQUIRES(!m_mutex)
      void get_allocator_block(this_thread_allocator_t &ta,
                               size_t create_sz,
                               size_t minimum_alloc_length,
                               size_t maximum_alloc_length,
                               size_t allocate_size,
                               block_type &destination);

      /**
       * \brief Release an interval of memory.
      **/
      void release_memory(const memory_pair_t &pair) REQUIRES(!m_mutex);
      /**
       * \brief Return true if the interval of memory is in the free list.
       **/
      REQUIRES(!m_mutex) auto in_free_list(const memory_pair_t &pair) const noexcept -> bool;
      /**
       * \brief Return length of free list.
       **/
      REQUIRES(!m_mutex) auto free_list_length() const noexcept -> size_t;
      /**
       * \brief Instead of destroying a block that is still in use, release to global allocator control.
      **/
      void to_global_allocator_block(block_type &&block) REQUIRES(!m_mutex);
      /**
       * \brief Destroy an allocator block.
      **/
      void destroy_allocator_block(this_thread_allocator_t &ta, block_type &&block) REQUIRES(!m_mutex);
      /**
       * \brief Register a allocator block before moving/destruction.
      **/
      void register_allocator_block(this_thread_allocator_t &ta, block_type &block) REQUIRES(!m_mutex);
      /**
       * \brief Unregister a registered allocator block before moving/destruction.
      **/
      void unregister_allocator_block(this_thread_allocator_t &ta, block_type &block) REQUIRES(!m_mutex);
      /**
       * \brief Move registered allocator blocks in container by offset.
      **/
      template <typename Container>
      void _u_move_registered_blocks(const Container &blocks, size_t offset) REQUIRES(m_mutex);
      void _u_move_registered_block(block_type *old_block, block_type *new_block) REQUIRES(m_mutex);
      /**
       * \brief Find the block for this allocator handle.
       *
       * Note that another thread could move the allocator block.
       * However, his can't happen without registering another block.
       * Therefore, the handle is only valid during this lock.
       * It is important to note this does not throw on error.  Instead it returns nullptr.
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
      Collapse the free list.
      **/
      void collapse() REQUIRES(!m_mutex);
      /**
       * \brief Return a reference to the spinlock.
       **/
      spinlock_t &_mutex() RETURN_CAPABILITY(m_mutex)
      {
        return m_mutex;
      }
      /**
       * \brief Destroy all memory pairs in the container.
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

      uint8_t *_u_begin() const noexcept;
      uint8_t *_u_current_end() const REQUIRES(m_mutex);
      uint8_t *_u_end() const noexcept;
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
       * \brief Return number of global blocks.
      **/
      size_t num_global_blocks() REQUIRES(!m_mutex);
      /**
       * \brief Return number of global blocks without locking.
      **/
      size_t _u_num_global_blocks() REQUIRES(m_mutex);

    private:
      using global_block_vector_type = rebind_vector_t<block_type, allocator>;
      /**
       * \brief Register a allocator block before moving/destruction without locking.
      **/
      void _u_register_allocator_block(this_thread_allocator_t &ta, block_type &block) REQUIRES(m_mutex);
      /**
       * \brief Return an allocator block.
      **/
      REQUIRES(!m_mutex) auto _create_allocator_block(this_thread_allocator_t &ta,
                                                      size_t sz,
                                                      size_t minimum_alloc_length,
                                                      size_t maximum_alloc_length) -> block_type;

      REQUIRES(m_mutex) auto _u_find_global_allocator_block(size_t sz, size_t minimum_alloc_length, size_t maximum_alloc_length)
          -> typename global_block_vector_type::iterator;
      /**
       * \brief Unregister a registered allocator block before moving/destruction without locking.
      **/
      void _u_unregister_allocator_block(this_thread_allocator_t &ta, block_type &block) REQUIRES(m_mutex);

      mutable spinlock_t m_mutex;
      /**
       * \brief This is set during destruction.
       *
       * Certain features of the allocator need to behave differently during destruction.
      **/
      std::atomic<bool> m_shutdown;
      /**
       * \brief Free interval vector.
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
      using thread_allocator_unique_ptr_t =
          typename ::std::unique_ptr<this_thread_allocator_t,
                                     typename cgc_allocator_deleter_t<this_thread_allocator_t, allocator>::type>;
      using ta_map_allocator_t = typename allocator::template rebind<
          typename ::std::pair<const typename ::std::thread::id, thread_allocator_unique_ptr_t>>::other;
      /**
       * \brief Blocks currently in use.
      **/
      rebind_vector_t<this_allocator_block_handle_t, allocator> m_blocks;
      /**
       * \brief Blocks currently held at the global allocator scope.
      **/
      global_block_vector_type m_global_blocks;
      /**
       * \brief Map from thread ids to thread allocators.
      **/
      ::std::map<::std::thread::id, thread_allocator_unique_ptr_t, ::std::less<::std::thread::id>, ta_map_allocator_t>
          m_thread_allocators GUARDED_BY(m_mutex);

      /**
       * \brief Allocator traits.
      **/
      Allocator_Traits m_traits;

    public:
      /**
       * \brief Internal function to return blocks currently in use.
      **/
      auto _u_blocks() -> decltype(m_blocks) &;
    };
  }
}
#include "thread_allocator_impl.hpp"
#include "allocator_impl.hpp"
