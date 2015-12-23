#pragma once
namespace mcppalloc
{
  namespace details
  {
    using user_data_alignment_t = size_t;
    /**
     * \brief Base class for user data associated with a entry.
     **/
    class alignas(user_data_alignment_t) user_data_base_t
    {
    public:
      user_data_base_t() = default;
      user_data_base_t(const user_data_base_t &) = default;
      user_data_base_t(user_data_base_t &&) = default;

    private:
      /**
       * \brief Magic constant that is used to check if this is user data.
       **/
      static const constexpr size_t c_magic_constant = 0x8e6866a0;
      /**
       * \brief Store a magic constant.
       *
       * If this magic constant is present, this is probabilistically user data.
       **/
      size_t m_magic_constant = c_magic_constant;

    public:
      auto magic_constant() const noexcept -> size_t
      {
        return m_magic_constant & (::std::numeric_limits<size_t>::max() ^ 1);
      }
      /**
       * \brief Return true if magic constant is valid.
       **/
      auto is_magic_constant_valid() const noexcept -> bool
      {
        return magic_constant() == c_magic_constant;
      }
      auto is_default() const noexcept -> bool
      {
        return (m_magic_constant & 1) == 1;
      }
      void set_is_default(bool is_default) noexcept
      {
        m_magic_constant = magic_constant() | static_cast<size_t>(is_default);
      }

    public:
      /**
       * True if is default, false otherwise.
      **/
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
    inline bool is_valid_object_state(const ::mcppalloc::details::object_state_t<Allocator_Policy> *state,
                                      const uint8_t *user_data_range_begin,
                                      const uint8_t *user_data_range_end);
  }
}
