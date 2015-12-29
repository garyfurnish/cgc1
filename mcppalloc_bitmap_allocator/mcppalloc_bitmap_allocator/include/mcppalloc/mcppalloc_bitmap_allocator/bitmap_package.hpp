#pragma once
#include "bitmap_state.hpp"
#include "declarations.hpp"
#include <boost/property_tree/ptree_fwd.hpp>
#include <mcppalloc/mcppalloc_utils/container.hpp>
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {
      template <typename Allocator_Policy>
      class bitmap_allocator_t;
      /**
       * \brief Return the id for an object of a given size.
       **/
      extern size_t get_bitmap_size_id(size_t sz);

      /**
       * \brief Collection of packed object states of various sizes.
       **/
      template <typename Allocator_Policy>
      class bitmap_package_t
      {
      public:
        static constexpr const size_t cs_total_size = c_bitmap_block_size;
        /**
         * \brief Number of vectors indexed by id.
         **/
        static constexpr const size_t cs_num_vectors = 5;
        using allocator_policy_type = Allocator_Policy;
        using internal_allocator_type = typename allocator_policy_type::internal_allocator_type;
        /**
         * \brief Type of vector holding states of a given id.
         **/
        using vector_type = rebind_vector_t<bitmap_state_t *, internal_allocator_type>;
        using free_list_type = rebind_vector_t<void *, internal_allocator_type>;
        /**
         * \brief Type of id indexed array.
         **/
        using array_type = ::std::array<vector_type, cs_num_vectors>;

        bitmap_package_t(type_id_t type_id);
        /**
         * \brief Copy constructor.
         *
         * Would prefer to delete this, but used internally by containers.
         **/
        bitmap_package_t(const bitmap_package_t &) = default;
        bitmap_package_t(bitmap_package_t &&) noexcept = default;
        /**
         * \brief Copy operator.
         *
         * Would prefer to delete this, but used internally by containers.
         **/
        //        bitmap_package_t &operator=(const bitmap_package_t &) = default;
        bitmap_package_t &operator=(bitmap_package_t &&) = default;
        /**
         * \brief Return type id.
         **/
        auto type_id() const noexcept -> type_id_t;
        void shutdown();
        /**
         * \brief Move all states in package into this package.
         **/
        void insert(bitmap_package_t &&state);
        /**
         * \brief Insert state into package.
         * @param id Id of the state.
         * @param state State to insert.
         **/
        void insert(size_t id, bitmap_state_t *state);
        /**
         * \brief Remove state with id from package.
         * @param id Size id for state.
         * @param state State to remove.
         * @return True on success, false otherwise.
         **/
        auto remove(size_t id, bitmap_state_t *state) -> bool;
        /**
         * \brief Allocate an object with given id.
         **/
        auto allocate(size_t id) noexcept -> ::std::pair<void *, size_t>;
        /**
         * \brief Do Maintenance for package.
         **/
        void do_maintenance(free_list_type &free_list) noexcept;

        template <typename Predicate>
        void for_all(Predicate &&predicate);

        /**
         * \brief Put debug information into a property tree.
         * @param level Level of information to give.  Higher is more verbose.
         **/
        void to_ptree(::boost::property_tree::ptree &ptree, int level) const;

        /**
         * \brief Allocate an object with given size.
         **/
        static bitmap_state_info_t _get_info(size_t id, type_id_t type_id = 0);
        /**
         * \brief Underlying states.
         **/
        array_type m_vectors;
        /**
         * \brief Type id of package.
         *
         * This should be treated as const.
         **/
        type_id_t m_type_id;
      };
    }
  }
}
#include "bitmap_package_impl.hpp"
