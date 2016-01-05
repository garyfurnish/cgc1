#pragma once
#include "slab_allocator_dll.hpp"
#include <boost/property_tree/ptree_fwd.hpp>
#include <mcppalloc/mcppalloc_utils/alignment.hpp>
#include <mcppalloc/mcppalloc_utils/concurrency.hpp>
#include <mcppalloc/mcppalloc_utils/function_iterator.hpp>
#include <mcppalloc/mcppalloc_utils/posix_slab.hpp>
#include <mcppalloc/mcppalloc_utils/win32_slab.hpp>
#include <mcppalloc/object_state.hpp>
namespace mcppalloc
{
  namespace slab_allocator
  {
    namespace details
    {
      using mutex_type = mutex_t;
      using slab_allocator_object_t = ::mcppalloc::details::object_state_t<default_allocator_policy_t<void>>;
      static_assert(::std::is_pod<slab_allocator_object_t>::value, "slab_allocator_object_t is not POD");
      /**
       * \brief This is a thread safe reentrant* slab allocator.
       *
       * This is essentially a thin layer over slab.
       * It is required that the lock not be held for reenterancy to be true.
       * It differs from allocator_t in that this allocator is completely in place.
       * This is both less memory and runtime efficient, but can be used to back the allocator_t.
      **/
      class MCPPALLOC_SLAB_ALLOCATOR_DLL_PUBLIC slab_allocator_t
      {
      public:
        static constexpr const size_t cs_alignment = 32;
        static constexpr const size_t cs_header_sz = align(sizeof(slab_allocator_object_t), cs_alignment);
        static constexpr size_t alignment() noexcept;
        static_assert(cs_header_sz == 64, "");
        /**
         * \brief Constructor.
         *
         * @param size Minimum size.
         * @param size_hint Suggested expand size.
         **/
        slab_allocator_t(size_t size, size_t size_hint);
        slab_allocator_t(const slab_allocator_t &) = delete;
        slab_allocator_t(slab_allocator_t &&) = delete;
        ~slab_allocator_t();
        void _verify() REQUIRES(!m_mutex);
        /**
         * \brief Align the next allocation to the given size.
         * This is only guarenteed to work if done before any deallocation.
         **/
        void align_next(size_t sz) REQUIRES(!m_mutex);
        /**
         * \brief Return start address of memory slab.
         **/
        uint8_t *begin() const;
        /**
         * \brief Return end address of memory slab.
         **/
        uint8_t *end() const;
        /**
         * \brief Object state begin iterator.
         **/
        next_iterator<slab_allocator_object_t> _u_object_begin();
        /**
         * \brief Object state end iterator.
         **/
        next_iterator<slab_allocator_object_t> _u_object_end();
        /**
         * \brief Object state current end iterator.
         **/
        next_iterator<slab_allocator_object_t> _u_object_current_end();
        /**
         * \brief Return true if no memory allocated.
         **/
        bool _u_empty();
        /**
         * \brief Return lock for this allocator.
         **/
        mutex_t &_mutex() RETURN_CAPABILITY(m_mutex)
        {
          return m_mutex;
        }
        /**
         * \brief Split an allocator object state.
         *
         * Requires holding lock.
         * @param object Object to split
         * @param sz Size of object allocation required.
         * @return Start of object memory.
         **/
        void *_u_split_allocate(slab_allocator_object_t *object, size_t sz);
        /**
         * \brief Allocate memory at end.
         *
         * Requires holding lock.
         * @param sz Size of object allocation required.
         * @return Start of object memory.
         **/
        void *_u_allocate_raw_at_end(size_t sz);
        /**
         * \brief Allocate memory.
         *
         * Requires holding lock.
         * @param sz Size of object allocation required.
         * @return Start of object memory.
         **/
        void *allocate_raw(size_t sz) REQUIRES(!m_mutex);
        /**
         * \brief Allocate memory.
         *
         * Deallocate memory.
         * @param v Memory to deallocate.
         **/
        void deallocate_raw(void *v) REQUIRES(!m_mutex);
        /**
         * \brief Return offset from start of slab for pointer.
         **/
        ptrdiff_t offset(void *v) const noexcept;
        /**
         * \brief Return currently used size.
         **/
        auto current_size() const noexcept -> size_t;
        /**
         * \brief Put information about slab allocator into a property tree.
         * @param level Level of information to give.  Higher is more verbose.
         **/
        void to_ptree(::boost::property_tree::ptree &ptree, int level) const;

      private:
        /**
         * \brief Mutex for allocator.
         **/
        mutex_type m_mutex;
        /**
         * \brief Underlying slab.
         **/
        slab_t m_slab;
        /**
         * \brief Current position of end object state (invalid).
         **/
        slab_allocator_object_t *m_end;
      };
      constexpr size_t slab_allocator_t::alignment() noexcept
      {
        return cs_alignment;
      }
    }
  }
}
#include "slab_allocator_inlines.hpp"
