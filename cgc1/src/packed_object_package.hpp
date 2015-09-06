#pragma once
#include <cgc1/declarations.hpp>
#include "packed_object_state.hpp"
#include "internal_allocator.hpp"
namespace cgc1
{
  namespace details
  {
    using namespace literals;
    /**
     * \brief Universal block size.
     **/
    static constexpr const size_t c_packed_object_block_size = 4096 * 128;
    class packed_object_allocator_t;
    /**
     * \brief Return the id for an object of a given size.
     **/
    extern size_t get_packed_object_size_id(size_t sz);

    /**
     * \brief Collection of packed object states of various sizes.
     **/
    class packed_object_package_t
    {
    public:
      static constexpr const size_t cs_total_size = c_packed_object_block_size;
      /**
       * \brief Number of vectors indexed by id.
       **/
      static constexpr const size_t cs_num_vectors = 5;
      /**
       * \brief Type of vector holding states of a given id.
       **/
      using vector_type = cgc_internal_vector_t<packed_object_state_t *>;
      /**
       * \brief Type of id indexed array.
       **/
      using array_type = ::std::array<vector_type, cs_num_vectors>;

      /**
       * \brief Move all states in package into this package.
       **/
      void insert(packed_object_package_t &&state);
      /**
       * \brief Insert state into package.
       * @param id Id of the state.
       * @param state State to insert.
       **/
      void insert(size_t id, packed_object_state_t *state);
      /**
       * \brief Remove state with id from package.
       * @param id Size id for state.
       * @param state State to remove.
       * @return True on success, false otherwise.
       **/
      auto remove(size_t id, packed_object_state_t *state) -> bool;
      /**
       * \brief Allocate an object with given id.
       **/
      auto allocate(size_t id) noexcept -> void *;
      /**
       * \brief Do Maintenance for package.
       **/
      void do_maintenance(cgc_internal_vector_t<void *> &free_list) noexcept;
      /**
       * \brief Allocate an object with given size.
       **/
      constexpr static packed_object_state_info_t _get_info(size_t id);
      /**
       * \brief Underlying states.
       **/
      array_type m_vectors;
    };
    constexpr inline packed_object_state_info_t packed_object_package_t::_get_info(size_t id)
    {
      return packed_object_state_info_t{cs_total_size / ((1 + id) << 5) / 512, (1_sz << (5 + id)), {0, 0}};
    }
  }
}
#ifdef CGC1_INLINES
#include "packed_object_package_impl.hpp"
#endif
