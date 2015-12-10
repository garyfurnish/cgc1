#pragma once
#include "bitmap_thread_allocator.hpp"
#include "bitmap_package.hpp"
#include <mcppalloc/mcppalloc_slab_allocator/slab_allocator.hpp>
#include <mcppalloc/mcppalloc_utils/thread_local_pointer.hpp>
#include <mcppalloc/mcppalloc_utils/make_unique.hpp>
#include <mcppalloc/mcppalloc_utils/container.hpp>
#include <map>
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {

      template <typename Allocator_Policy>
      class bitmap_allocator_t
      {
      public:
        using mutex_type = mutex_t;
        using allocator_policy_type = Allocator_Policy;
        using allocator_thread_policy_type = typename Allocator_Policy::thread_policy_type;
        using thread_allocator_type = bitmap_thread_allocator_t<allocator_policy_type>;
        using internal_allocator_type = typename allocator_policy_type::internal_allocator_type;
        using slab_allocator_type = ::mcppalloc::slab_allocator::details::slab_allocator_t;

        bitmap_allocator_t(size_t size, size_t size_hint);
        bitmap_allocator_t(const bitmap_allocator_t &) = delete;
        bitmap_allocator_t(bitmap_allocator_t &&) noexcept = delete;
        bitmap_allocator_t &operator=(const bitmap_allocator_t &) = delete;
        bitmap_allocator_t &operator=(bitmap_allocator_t &&) noexcept = delete;
        ~bitmap_allocator_t();

        REQUIRES(!m_mutex) void shutdown();

        REQUIRES(!m_mutex) auto _get_memory() noexcept -> bitmap_state_t *;
        void _u_to_global(size_t id, type_id_t type, bitmap_state_t *state) noexcept REQUIRES(m_mutex);
        void _u_to_free(void *v) noexcept REQUIRES(m_mutex);
        /**
         * \brief Initialize or return thread allocator for this thread.
         **/
        REQUIRES(!m_mutex) auto initialize_thread() -> thread_allocator_type &;
        /**
         * \brief Destroy thread allocator for this thread.
         **/
        REQUIRES(!m_mutex) void destroy_thread();
        REQUIRES(!m_mutex) auto num_free_blocks() const noexcept -> size_t;
        REQUIRES(!m_mutex) auto num_globals(size_t id, type_id_t type) const noexcept -> size_t;

        RETURN_CAPABILITY(m_mutex) auto _mutex() const noexcept -> mutex_type &;

        auto begin() const noexcept -> uint8_t *;
        auto end() const noexcept -> uint8_t *;
        auto _slab() const noexcept -> slab_allocator_type &;

        // TODO: NOT SURE IF THIS SHOULD BE NO LOCK OR NOT... maybe we need to acquire it in gc?
        template <typename Predicate>
        NO_THREAD_SAFETY_ANALYSIS void _for_all_state(Predicate &&);

        /**
         * \brief Return reference to allocator policy.
         **/
        auto allocator_policy() noexcept -> allocator_thread_policy_type &;
        /**
         * \brief Return reference to allocator policy.
         **/
        auto allocator_policy() const noexcept -> const allocator_thread_policy_type &;
        /**
         * \brief Notify all threads to force maintenance.
         **/
        void _u_set_force_maintenance() REQUIRES(m_mutex);
        /**
         * \brief Put information about allocator into a property tree.
         * @param level Level of information to give.  Higher is more verbose.
         **/
        void to_ptree(::boost::property_tree::ptree &ptree, int level) const REQUIRES(!m_mutex);

        using package_type = bitmap_package_t<allocator_policy_type>;

      private:
        /**
         * \brief Encapsulate access to thread local variable for thread allocator.
         **/
        static auto get_ttla() noexcept -> thread_allocator_type *;
        /**
         * \brief Encapsulate access to thread local variable for thread allocator.
         **/
        static void set_ttla(thread_allocator_type *ta) noexcept;
        /**
         * \brief Per thread thread allocator variable.
         **/
        static thread_local_pointer_t<thread_allocator_type> t_thread_allocator;
        /**
         * \brief Type of unique ptr for thread allocator.
         **/
        using thread_allocator_unique_ptr_type =
            typename ::std::unique_ptr<thread_allocator_type,
                                       typename cgc_allocator_deleter_t<thread_allocator_type, internal_allocator_type>::type>;
        /**
         * \brief Mutex for allocator.
         **/
        mutable mutex_type m_mutex;
        /**
         * \brief Underlying slab.
         **/
        mutable slab_allocator_type m_slab;
        /**
         * \brief Allocator policy.
         **/
        allocator_thread_policy_type m_policy;
        /**
         * \brief In use states held by global.
         **/
        ::boost::container::flat_map<type_id_t,
                                     package_type,
                                     ::std::less<type_id_t>,
                                     typename ::std::allocator_traits<internal_allocator_type>::template rebind_alloc<
                                         ::std::pair<type_id_t, package_type>>> m_globals GUARDED_BY(m_mutex);
        using free_list_type = rebind_vector_t<void *, internal_allocator_type>;
        /**
         * \brief Free sections of slab.
         **/
        free_list_type m_free_globals GUARDED_BY(m_mutex);
        /**
         * \brief Thread allocators.
         **/
        ::boost::container::flat_map<::std::thread::id,
                                     thread_allocator_unique_ptr_type,
                                     ::std::less<::std::thread::id>,
                                     typename ::std::allocator_traits<internal_allocator_type>::template rebind_alloc<
                                         ::std::pair<::std::thread::id, thread_allocator_unique_ptr_type>>> m_thread_allocators;
      };
    }
    template <typename Allocator_Policy>
    using bitmap_allocator_t = details::bitmap_allocator_t<Allocator_Policy>;
  }
}
#include "bitmap_allocator_impl.hpp"
