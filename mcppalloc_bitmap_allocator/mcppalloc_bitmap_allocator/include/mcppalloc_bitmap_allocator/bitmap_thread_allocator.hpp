#pragma once
#include "declarations.hpp"
#include "bitmap_state.hpp"
#include "bitmap_package.hpp"
#include <mcppalloc/block.hpp>
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {
      template <typename Allocator_Policy>
      class bitmap_thread_allocator_t
      {
      public:
        using allocator_policy_type = Allocator_Policy;
        using package_type = bitmap_package_t<allocator_policy_type>;
	using block_type = block_t<allocator_policy_type>;
        bitmap_thread_allocator_t(bitmap_allocator_t<allocator_policy_type> &allocator);
        bitmap_thread_allocator_t(const bitmap_thread_allocator_t &) = delete;
        bitmap_thread_allocator_t(bitmap_thread_allocator_t &&) noexcept;
        bitmap_thread_allocator_t &operator=(const bitmap_thread_allocator_t &) = delete;
        bitmap_thread_allocator_t &operator=(bitmap_thread_allocator_t &&) = default;
        ~bitmap_thread_allocator_t();
        using internal_allocator_type = typename allocator_policy_type::internal_allocator_type;

        auto allocate(size_t sz) noexcept -> block_type;
        auto deallocate(void *v) noexcept -> bool;
        auto destroy(void *v) noexcept -> bool
        {
          return deallocate(v);
        }

        void set_max_in_use(size_t max_in_use);
        void set_max_free(size_t max_free);
        auto max_in_use() const noexcept -> size_t;
        auto max_free() const noexcept -> size_t;
        void do_maintenance();

        template <typename Predicate>
        void for_all_state(Predicate &&predicate);
        /**
         * \brief Tell thread to perform maintenance at next opportunity.
         **/
        void set_force_maintenance();

        /**
         * \brief Put information about allocator into a property tree.
         * @param level Level of information to give.  Higher is more verbose.
         **/
        void to_ptree(::boost::property_tree::ptree &ptree, int level);

      private:
        void _check_maintenance();
        package_type m_locals;
        ::std::atomic<bool> m_force_maintenance;
        rebind_vector_t<void *, internal_allocator_type> m_free_list;
        bitmap_allocator_t<allocator_policy_type> &m_allocator;
        ::std::array<size_t, package_type::cs_num_vectors> m_popcount_max;
        size_t m_max_in_use{10};
        size_t m_max_free{5};
        bool m_in_destructor{false};
      };
    }
  }
}
#include "bitmap_thread_allocator_impl.hpp"
