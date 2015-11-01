#pragma once
namespace mcppalloc
{
  namespace sparse
  {
    namespace details
    {
      /**
       * \brief Structure containing information on an allocation failure.
       **/
      struct allocation_failure_t {
        size_t m_failures;
      };
      /**
       * \brief Structure containing information on what to do in responce to allocation failure.
       **/
      struct allocation_failure_action_t {
        /**
         * \brief True if an attempt should be made to expand underlying storage, otherwise false.
         **/
        bool m_attempt_expand;
        /**
         * \brief True if should attempt allocation again.
         **/
        bool m_repeat;
      };

      struct allocator_thread_policy_tag_t {
      };
    }
  }
}
