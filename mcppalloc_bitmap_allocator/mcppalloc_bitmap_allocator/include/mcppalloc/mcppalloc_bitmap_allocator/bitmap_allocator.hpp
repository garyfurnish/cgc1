#pragma once
#include "bitmap_package.hpp"
#include "bitmap_thread_allocator.hpp"
#include "bitmap_type_info.hpp"
#include <map>
#include <mcppalloc/mcppalloc_slab_allocator/slab_allocator.hpp>
#include <mcpputil/mcpputil/container.hpp>
#include <mcpputil/mcpputil/make_unique.hpp>
#include <mcpputil/mcpputil/thread_local_pointer.hpp>
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
        using mutex_type = mcpputil::mutex_t;
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

        REQUIRES(!m_mutex) auto _get_memory() -> bitmap_state_t *;
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
        /**
         * \brief Add an allocation type.
         *
         * This should not be called after first allocation.
         **/
        void add_type(bitmap_type_info_t &&type_info);
        /**
         * \brief Get type.
         *
         * Throws on error.
         **/
        auto get_type(type_id_t type_id) -> const bitmap_type_info_t &;

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
        static mcpputil::thread_local_pointer_t<thread_allocator_type> t_thread_allocator;
        /**
         * \brief Type of unique ptr for thread allocator.
         **/
        using thread_allocator_unique_ptr_type = typename ::std::unique_ptr<
            thread_allocator_type,
            typename mcpputil::cgc_allocator_deleter_t<thread_allocator_type, internal_allocator_type>::type>;
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
        using globals_pair_type = typename ::std::pair<type_id_t, package_type>;
        using internal_allocator_traits = typename ::std::allocator_traits<internal_allocator_type>;
        using globals_allocator_type = typename internal_allocator_traits::template rebind_alloc<globals_pair_type>;
        ::boost::container::flat_map<type_id_t, package_type, typename ::std::less<type_id_t>, globals_allocator_type>
            m_globals GUARDED_BY(m_mutex);
        using types_pair_type = ::std::pair<type_id_t, bitmap_type_info_t>;
        using types_allocator_type = typename internal_allocator_traits::template rebind_alloc<types_pair_type>;
        /**
         * \brief Type info for various allocation types.
         *
         * This should not be modified after first use as it is not thread safe for writing.
         **/
        ::boost::container::flat_map<type_id_t, bitmap_type_info_t, ::std::less<type_id_t>, types_allocator_type> m_types;
        /**
         * \brief Type of free list memory list.
         **/
        using free_list_type = mcpputil::rebind_vector_t<void *, internal_allocator_type>;
        /**
         * \brief Free sections of slab.
         **/
        free_list_type m_free_globals GUARDED_BY(m_mutex);
        using thread_allocators_pair_type = typename ::std::pair<::std::thread::id, thread_allocator_unique_ptr_type>;
        using thread_allocators_allocator_type =
            typename internal_allocator_traits::template rebind_alloc<thread_allocators_pair_type>;
        /**
         * \brief Thread allocators.
         **/
        ::boost::container::flat_map<::std::thread::id,
                                     thread_allocator_unique_ptr_type,
                                     ::std::less<::std::thread::id>,
                                     thread_allocators_allocator_type>
            m_thread_allocators;
      };
    }
    template <typename Allocator_Policy>
    using bitmap_allocator_t = details::bitmap_allocator_t<Allocator_Policy>;
  }
}
#include "bitmap_allocator_impl.hpp"
