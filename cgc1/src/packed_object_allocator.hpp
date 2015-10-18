#pragma once
#include "packed_object_thread_allocator.hpp"
#include "slab_allocator.hpp"
#include <cgc1/thread_local_pointer.hpp>
#include <map>
namespace cgc1
{
  namespace details
  {

    class gc_packed_object_allocator_policy_t
    {
    public:
      void on_allocate(void *addr, size_t sz);
      auto on_allocation_failure(const packed_allocation_failure_t &action) -> packed_allocation_failure_action_t;
    };
    template <typename Allocator_Policy>
    class packed_object_allocator_t
    {
    public:
      using mutex_type = mutex_t;
      using allocator_policy_type = Allocator_Policy;
      using thread_allocator_type = packed_object_thread_allocator_t<allocator_policy_type>;
      using allocator = cgc_internal_allocator_t<void>;

      packed_object_allocator_t(size_t size, size_t size_hint);
      packed_object_allocator_t(const packed_object_allocator_t &) = delete;
      packed_object_allocator_t(packed_object_allocator_t &&) noexcept = delete;
      packed_object_allocator_t &operator=(const packed_object_allocator_t &) = delete;
      packed_object_allocator_t &operator=(packed_object_allocator_t &&) noexcept = delete;
      ~packed_object_allocator_t();
      REQUIRES(!m_mutex) auto _get_memory() noexcept -> packed_object_state_t *;
      void _u_to_global(size_t id, packed_object_state_t *state) noexcept REQUIRES(m_mutex);
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
      REQUIRES(!m_mutex) auto num_globals(size_t id) const noexcept -> size_t;

      RETURN_CAPABILITY(m_mutex) auto _mutex() const noexcept -> mutex_type &;

      auto begin() const noexcept -> uint8_t *;
      auto end() const noexcept -> uint8_t *;
      auto _slab() const noexcept -> slab_allocator_t &;

      // TODO: NOT SURE IF THIS SHOULD BE NO LOCK OR NOT... maybe we need to acquire it in gc?
      template <typename Predicate>
      NO_THREAD_SAFETY_ANALYSIS void _for_all_state(Predicate &&);

      /**
       * \brief Return reference to allocator policy.
       **/
      auto allocator_policy() noexcept -> allocator_policy_type &;
      /**
       * \brief Return reference to allocator policy.
       **/
      auto allocator_policy() const noexcept -> const allocator_policy_type &;
      /**
       * \brief Notify all threads to force maintenance.
       **/
      void _u_set_force_maintenance() REQUIRES(m_mutex);
      /**
       * \brief Put information about allocator into a property tree.
       * @param level Level of information to give.  Higher is more verbose.
       **/
      void to_ptree(::boost::property_tree::ptree &ptree, int level) const REQUIRES(!m_mutex);

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
                                     typename cgc_allocator_deleter_t<thread_allocator_type, allocator>::type>;
      /**
       * \brief Mutex for allocator.
       **/
      mutable mutex_type m_mutex;
      /**
       * \brief Underlying slab.
       **/
      mutable slab_allocator_t m_slab;
      /**
       * \brief Allocator policy.
       **/
      allocator_policy_type m_policy;
      packed_object_package_t m_globals GUARDED_BY(m_mutex);
      /**
       * \brief Free sections of slab.
       **/
      cgc_internal_vector_t<void *> m_free_globals GUARDED_BY(m_mutex);
      /**
       * \brief Thread allocators.
       **/
      ::std::map<::std::thread::id, thread_allocator_unique_ptr_type> m_thread_allocators;
    };
  }
}
#include "packed_object_allocator_inlines.hpp"
#include "packed_object_allocator_impl.hpp"
