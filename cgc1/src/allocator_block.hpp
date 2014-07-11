#pragma once
#include "internal_declarations.hpp"
#include <vector>
#include <algorithm>
#include "allocator_block.hpp"
#include "object_state.hpp"
namespace cgc1
{
  template <typename T, typename allocator>
  using rebind_vector_t = typename ::std::vector<T, typename allocator::template rebind<T>::other>;

  namespace details
  {

    template <typename Allocator, typename User_Data = user_data_base_t>
    class allocator_block_t
    {
    public:
      using allocator = Allocator;
      using user_data_type = User_Data;
      /**
      @param start Start of memory block that this allocator uses.
      @param length Length of memory block that this allocator uses
      @param minimum_alloc_length Minimum Length of object that can be allocated using this allocator.
      **/
      allocator_block_t(void *start, size_t length, size_t minimum_alloc_length);
      allocator_block_t(const allocator_block_t &) = delete;
      allocator_block_t(allocator_block_t &&);
      allocator_block_t &operator=(allocator_block_t &) = delete;
      allocator_block_t &operator=(allocator_block_t &&);
      /**
      Return true if no items allocated.
      Return false if items are allocated or if no items are allocated and a connection is needed.
      Note the semantics here!
      **/
      bool empty() const;
      /**
      Return true if no more allocations can be performed.
      **/
      bool full() const;
      /**
      Begin iterator to allow bytewise iteration over memory block.
      **/
      uint8_t *begin() const;
      /**
      End iterator to allow bytewise iteration over memory block.
      **/
      uint8_t *end() const;
      /**
      End iterator for object_states
      **/
      object_state_t *current_end() const;
      /**
      Return beginning object state. 
      Produces undefined behavior if no valid object states.
      **/
      object_state_t *_object_state_begin() const;
      /**
      Find the object state associated with the given address.
      Return nullptr if not found.
      **/
      object_state_t *find_address(void* addr) const;
      /**
      Allocate size bytes on the block, return nullptr if not possible.
      **/
      void *allocate(size_t size);
      /**
      Destroy a v that is on the block.
      Return true on success, false on failure.
      The usual cause of failure would be the pointer not being in the block.
      **/
      bool destroy(void *v);
      /**
      Collect any adjacent blocks that may have formed into one block.
      **/
      void collect();
      /**
      Return the maximum allocation size available.
      **/
      size_t max_alloc_available() const;
      void _verify(const object_state_t *os);

      bool valid() const;
      void clear();

    public:
      /// free list for this blcok.
      rebind_vector_t<object_state_t *, allocator> m_free_list;
      /// next allocator pointer if whole block has not yet been used.
      object_state_t *m_next_alloc_ptr;
      /// End of memory block
      uint8_t *m_end;
      /// minimum object allocation length.
      size_t m_minimum_alloc_length;
      /// start of memory block.
      uint8_t *m_start;

      unique_ptr_allocated<user_data_type,Allocator> m_default_user_data;
    };
    class user_data_base_t
    {
    public:
      user_data_base_t() = delete;
      template <typename Allocator, typename User_Data>
      user_data_base_t(allocator_block_t<Allocator,User_Data>* block);
      user_data_base_t(const user_data_base_t&) = default;
      user_data_base_t(user_data_base_t&&) = default;
      static constexpr size_t c_magic_constant = 0x8e6866a1;
      size_t m_magic_constant = c_magic_constant;
      bool is_magic_constant_valid() const { return m_magic_constant == c_magic_constant; }
      uint8_t* allocator_block_begin() const { return m_allocator_block_begin; }
      object_state_t* allocator_block_state_begin() const { return reinterpret_cast<object_state_t*>(allocator_block_begin()); }
    private:
      uint8_t *m_allocator_block_begin;
    public:
      /**
      True if is default, false otherwise.
      **/
      bool m_is_default = false;
    };
    class gc_user_data_t : public user_data_base_t
    {
    public:
      template <typename Allocator, typename User_Data>
      gc_user_data_t(allocator_block_t<Allocator,User_Data>* block);
      /**
      Optional finalizer function to run.
      **/
      ::std::function<void(void *)> m_finalizer = nullptr;
      /**
      True if uncollectable, false otherwise.
      **/
      bool m_uncollectable = false;
      ::std::string m_file;
      unsigned long m_line = 0;
      size_t test = 0x5000;

    private:
    };
    /**
    Return true if the object state is a valid object state, false otherwise.
    Uses magic user data detection.
    @param state State to test for validity.
    @param user_data_range_begin Beginning of the memory range that is a valid location for user data.
    @param user_data_range_end End of the memory range that is a valid location for user data.
    **/
    inline bool is_valid_object_state(const object_state_t* state, const uint8_t* user_data_range_begin, const uint8_t* user_data_range_end);
  }
}
#include "allocator_block_impl.hpp"
