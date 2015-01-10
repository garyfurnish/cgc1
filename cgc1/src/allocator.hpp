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
    Structure used to store data about an allocation block in an allocator.
    **/
    template <typename Global_Allocator>
    struct allocator_block_handle_t {
      using global_allocator_t = Global_Allocator;
      using block_type = typename global_allocator_t::block_type;
      allocator_block_handle_t(typename global_allocator_t::this_thread_allocator_t *ta, block_type *block, uint8_t *begin)
          : m_thread_allocator(ta), m_block(block), m_begin(begin)
      {
      }
      allocator_block_handle_t(const allocator_block_handle_t &) = default;
      allocator_block_handle_t(allocator_block_handle_t &&) = default;
      allocator_block_handle_t &operator=(const allocator_block_handle_t &) = default;
      allocator_block_handle_t &operator=(allocator_block_handle_t &&) = default;
      bool operator==(const allocator_block_handle_t &b) const
      {
        return m_thread_allocator == b.m_thread_allocator && m_block == b.m_block;
      }
      /**
      Thread allocator that currently owns this handle.
      May be nullptr.
      **/
      typename global_allocator_t::this_thread_allocator_t *m_thread_allocator;
      /**
      Address of block for this handle.
      **/
      block_type *m_block;
      /**
      Since allocator_blocks may be temporarily inconsistent during a move operation, we cash their beginning location.
      **/
      uint8_t *m_begin;
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
      Initialize the allocator.
      @param initial_gc_heap_size Initial size of gc heap.
      @param suggested_max_heap_size Hint about how large the gc heap may grow.
      @return True on success, false on failure.
      **/
      bool initialize(size_t initial_gc_heap_size, size_t suggested_max_heap_size) REQUIRES(!m_mutex);
      this_thread_allocator_t &initialize_thread() REQUIRES(!m_mutex);
      void destroy_thread() REQUIRES(!m_mutex);
      /**
      Get an interval of memory.
      Returns (nullptr,nullptr) on error.
      **/
      memory_pair_t get_memory(size_t sz) REQUIRES(!m_mutex);
      /**
      Return an allocator block.
      **/
      template <typename... Args>
      block_type create_allocator_block(this_thread_allocator_t &ta, size_t sz, Args &&... args) REQUIRES(!m_mutex);
      /**
      Release an interval of memory.
      **/
      void release_memory(const memory_pair_t &pair) REQUIRES(!m_mutex);
      /**
      Instead of destroying a block that is still in use, release to global allocator control.
      **/
      void to_global_allocator_block(block_type &&block) REQUIRES(!m_mutex);
      /**
      Destroy an allocator block.
      **/
      void destroy_allocator_block(this_thread_allocator_t &ta, block_type &&block) REQUIRES(!m_mutex);
      /**
      Register a allocator block before moving/destruction.
      **/
      void register_allocator_block(this_thread_allocator_t &ta, block_type &block) REQUIRES(!m_mutex);
      /**
      Unregister a registered allocator block before moving/destruction.
      **/
      void unregister_allocator_block(this_thread_allocator_t &ta, block_type &block) REQUIRES(!m_mutex);
      /**
      Move registered allocator blocks in container by offset.
      **/
      template <typename Container>
      void _u_move_registered_blocks(const Container &blocks, size_t offset) REQUIRES(m_mutex);
      void _u_move_registered_block(block_type *old_block, block_type *new_block);
      /**
      Find the block for this allocator handle.
      Note that another thread could move the allocator block, but this can't happen without registering another block, assuming
      the block interface is used.
      Therefore, the handle is only valid during this lock.
      Return nullptr on error.
      **/
      const this_allocator_block_handle_t *_u_find_block(void *addr) REQUIRES(m_mutex);
      /**
      Return the beginning of the underlying slab.
      **/
      uint8_t *begin() const REQUIRES(!m_mutex);
      /**
      Return the end of the underlying slab.
      **/
      uint8_t *end() const REQUIRES(!m_mutex);
      /**
      Return the end of the currently used portion of the slab.
      **/
      uint8_t *current_end() const REQUIRES(!m_mutex);
      /**
      Collapse the free list.
      **/
      void collapse() REQUIRES(!m_mutex);
      spinlock_t &_mutex() RETURN_CAPABILITY(m_mutex)
      {
        return m_mutex;
      }
      template <typename Container>
      void bulk_destroy_memory(Container &container) REQUIRES(!m_mutex);
      /**
      Return the free list for debugging purposes.
      **/
      memory_pair_vector_t _d_free_list() const REQUIRES(!m_mutex);
      /**
      Return the free list for debugging purposes.
      **/
      const memory_pair_vector_t &_ud_free_list() const REQUIRES(m_mutex);

      uint8_t *_u_begin() const NO_THREAD_SAFETY_ANALYSIS;
      uint8_t *_u_current_end() const NO_THREAD_SAFETY_ANALYSIS;

      mutable spinlock_t m_mutex;
      /**
      Internal consistency checks.
      **/
      void _ud_verify();
      /**
      Internal consistency checks.
      **/
      void _d_verify() REQUIRES(!m_mutex);

      bool is_shutdown() const;

    private:
      std::atomic<bool> m_shutdown;
      /**
      Free interval vector.
      **/
      memory_pair_vector_t m_free_list GUARDED_BY(m_mutex);
      /**
      Pointer to end of currently used portion of slab.
      **/
      uint8_t *m_current_end GUARDED_BY(m_mutex) = nullptr;
      /**
      Initial size of gc heap.
      **/
      size_t m_initial_gc_heap_size GUARDED_BY(m_mutex) = 0;
      /**
      Minimum size to expand heap by.
      **/
      size_t m_minimum_expansion_size GUARDED_BY(m_mutex) = 0;
      /**
      Underlying slab.
      **/
      slab_t m_slab GUARDED_BY(m_mutex);
      using thread_allocator_unique_ptr_t =
          typename ::std::unique_ptr<this_thread_allocator_t,
                                     typename cgc_allocator_deleter_t<this_thread_allocator_t, allocator>::type>;
      using ta_map_allocator_t = typename allocator::template rebind<
          typename ::std::pair<const typename ::std::thread::id, thread_allocator_unique_ptr_t>>::other;
      /**
      Blocks currently in use.
      **/
      rebind_vector_t<this_allocator_block_handle_t, allocator> m_blocks;
      /**
      Blocks currently held at the global allocator scope.
      **/
      rebind_vector_t<block_type, allocator> m_global_blocks;
      /**
      Map from thread ids to thread allocators.
      **/
      ::std::map<::std::thread::id, thread_allocator_unique_ptr_t, ::std::less<::std::thread::id>, ta_map_allocator_t>
          m_thread_allocators GUARDED_BY(m_mutex);

      /**
      Allocator traits.
      **/
      Allocator_Traits m_traits;

    public:
      /**
      Return blocks currently in use.
      **/
      auto _u_blocks() -> decltype(m_blocks) &;
    };
  }
}
#include "thread_allocator_impl.hpp"
#include "allocator_impl.hpp"
