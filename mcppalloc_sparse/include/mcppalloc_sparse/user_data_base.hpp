#pragma once
namespace mcppalloc
{
  namespace sparse
  {
    namespace details
    {
      /**
       * \brief Base class for user data associated with a entry.
       **/
      class user_data_base_t
      {
      public:
        user_data_base_t() = default;
        user_data_base_t(const user_data_base_t &) = default;
        user_data_base_t(user_data_base_t &&) = default;

      private:
        /**
         * \brief Magic constant that is used to check if this is user data.
         **/
        static constexpr size_t c_magic_constant = 0x8e6866a1;
        /**
         * \brief Store a magic constant.
         *
         * If this magic constant is present, this is probabilistically user data.
         **/
        size_t m_magic_constant = c_magic_constant;

      public:
        /**
         * \brief Return true if magic constant is valid.
         **/
        bool is_magic_constant_valid() const
        {
          return m_magic_constant == c_magic_constant;
        }

      public:
        /**
         * True if is default, false otherwise.
        **/
        bool m_is_default = false;
        /**
         * \brief True if uncollectable, false otherwise.
         *
         * Only used for gc_type.
        **/
        bool m_uncollectable = false;
      };
      /**
         * \brief User data that can be associated with an allocation.
         **/
      class gc_user_data_t : public user_data_base_t
      {
      public:
        /**
         * \brief Constructor
         *
         * @param block Block that this allocation belongs to.
         **/
        gc_user_data_t() = default;
        /**
         * \brief Optional finalizer function to run.
        **/
        ::std::function<void(void *)> m_finalizer = nullptr;

      private:
      };
      /**
       * \brief Return true if the object state is a valid object state, false otherwise.
       *
       * Uses magic user data detection.
       * @param state State to test for validity.
       * @param user_data_range_begin Beginning of the memory range that is a valid location for user data.
       * @param user_data_range_end End of the memory range that is a valid location for user data.
      **/
      template <typename Allocator_Policy>
      inline bool is_valid_object_state(const object_state_t<Allocator_Policy> *state,
                                        const uint8_t *user_data_range_begin,
                                        const uint8_t *user_data_range_end);
    }
  }
}
