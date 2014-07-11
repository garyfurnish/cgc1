#pragma once
#include "object_state.hpp"
#include "allocator_block_set.hpp"
#include <cgc1/posix_slab.hpp>
#include <array>
namespace cgc1
{
  namespace details
  {
    /**
    Treating the pair as a beginning and end, return the size of the memory block between them.
    **/
    inline ::std::ptrdiff_t size(const ::std::pair<uint8_t *, uint8_t *> &pair);
    /**
    Treating the pair as a beginning and end, return the positive size of the memory block between them.
    **/
    inline size_t size_pos(const ::std::pair<uint8_t *, uint8_t *> &pair);
    /**
    Output the memory pair as a stirng to the stream.
    **/
    template <typename T>
    void print_memory_pair(T &os, const ::std::pair<uint8_t *, uint8_t *> &pair);
    /**
    Output the memory pair to the string.
    **/
    inline ::std::string to_string(const ::std::pair<uint8_t *, uint8_t *> &pair);
    template <typename Allocator, typename Traits>
    class allocator_t;
    /**
    Per thread allocator for a global slab allocator.
    **/
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    class thread_allocator_t
    {
    public:
      using allocator = Allocator;
      using global_allocator = Global_Allocator;
      using allocator_traits = Allocator_Traits;
      using this_allocator_block_set_t =
          allocator_block_set_t<allocator, typename allocator_traits::allocator_block_user_data_type>;
      /**
      Number of allocator size bins.
      **/
      static constexpr const size_t c_bins = 18;
      /**
      Constructor.
      @param allocator Global allocator for slabs.
      **/
      thread_allocator_t(global_allocator &allocator);
      thread_allocator_t(thread_allocator_t<global_allocator, allocator, allocator_traits> &) = delete;
      thread_allocator_t(thread_allocator_t<global_allocator, allocator, allocator_traits> &&ta) = default;
      thread_allocator_t &operator=(const thread_allocator_t<global_allocator, allocator, allocator_traits> &) = delete;
      thread_allocator_t &operator=(thread_allocator_t<global_allocator, allocator, allocator_traits> &&) = default;
      ~thread_allocator_t();
      /**
      Return the bin id for a given size.
      **/
      static size_t find_block_set_id(size_t sz);
      /**
      Fill mutiple array with reasonable default values.
      **/
      void fill_multiples_with_default_values();
      /**
      Set the allocation multiple for a given allocator bin.
      **/
      bool set_allocator_multiple(size_t id, size_t multiple);
      /**
      Return the allocation multiple for a given allocator bin.
      **/
      size_t get_allocator_multiple(size_t id);
      /**
      Return the block size for a given allocator bin.
      **/
      size_t get_allocator_block_size(size_t id) const;
      /**
      Return a reference to the allocator for a given size.
      **/
      this_allocator_block_set_t &allocator_by_size(size_t sz);
      /**
      Return an array of block sizes for all of the bins.
      **/
      ::std::array<size_t, c_bins> allocator_block_sizes() const;
      /**
      Allocate memory of size.
      @return nullptr on error.
      **/
      void *allocate(size_t size);
      /**
      Destroy a pointer allocated by this allocator.
      The common reason for failure is if this allocator did not make the pointer.
      @return True on success, false on failure.
      **/
      bool destroy(void *v);
      /**
      Return the array of multiples for debugging purposes.
      **/
      auto allocator_multiples() const -> const ::std::array<size_t, c_bins> &;
      /**
      Return the array of allocators for debugging purposes.
      **/
      auto allocators() const -> const ::std::array<this_allocator_block_set_t, c_bins> &;
      /**
      Free all empty blocks back to allocator.
      **/
      void free_empty_blocks();

    private:
      /**
      Global allocator used for getting slabs.
      **/
      global_allocator &m_allocator;
      /**
      Allocator multiple for requesting new blocks.
      **/
      ::std::array<size_t, c_bins> m_allocator_multiples;
      /**
      Allocators used to allocate various sizes of memory.
      **/
      ::std::array<this_allocator_block_set_t, c_bins> m_allocators;
    };
    /**
    Stream output for debugging.
    **/
    template <typename charT, typename Traits, typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    ::std::basic_ostream<charT, Traits> &operator<<(::std::basic_ostream<charT, Traits> &os,
                                                    const thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits> &ta);
  }
}
#include "allocator.hpp"
