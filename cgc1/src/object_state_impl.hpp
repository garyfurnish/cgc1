namespace cgc1
{
  namespace details
  {
    CGC1_OPT_INLINE bool is_aligned_properly(const object_state_t *os) noexcept
    {
      return os == align_pow2(os, c_align_pow2);
    }
    CGC1_OPT_INLINE object_state_t *object_state_t::from_object_start(void *v) noexcept
    {
      return reinterpret_cast<object_state_t *>(reinterpret_cast<uint8_t *>(v) - align(sizeof(object_state_t)));
    }
    CGC1_OPT_INLINE void object_state_t::set_all(object_state_t *next, bool in_use, bool next_valid, bool quasi_freed) noexcept
    {
      m_next = ((size_t)next) | ((size_t)in_use) | (((size_t)next_valid) << 1 | (((size_t)quasi_freed) << 2));
    }
    CGC1_OPT_INLINE void object_state_t::set_in_use(bool v) noexcept
    {
      size_t ptr = (size_t)m_next;
      size_t iv = (size_t)v;
      m_next = (ptr & size_inverse(1)) | (iv & 1);
    }
    CGC1_OPT_INLINE bool object_state_t::not_available() const noexcept
    {
      return 0 < (m_next & 5);
    }
    CGC1_OPT_INLINE bool object_state_t::in_use() const noexcept
    {
      return m_next & 1;
    }
    CGC1_OPT_INLINE bool object_state_t::quasi_freed() const noexcept
    {
      return (m_next & 4) > 0;
    }
    CGC1_OPT_INLINE void object_state_t::set_quasi_freed() noexcept
    {
      set_all(next(), false, next_valid(), true);
    }
    CGC1_OPT_INLINE void object_state_t::set_quasi_freed(bool val) noexcept
    {
      assert(!(val && in_use()));
      set_all(next(), in_use(), next_valid(), val);
    }
    CGC1_OPT_INLINE void object_state_t::set_next_valid(bool v) noexcept
    {
      size_t ptr = (size_t)m_next;
      size_t iv = ((size_t)v) << 1;
      m_next = (ptr & size_inverse(2)) | (iv & 2);
    }
    CGC1_OPT_INLINE bool object_state_t::next_valid() const noexcept
    {
      return (((size_t)m_next) & 2) > 0;
    }
    CGC1_OPT_INLINE object_state_t *object_state_t::next() const noexcept
    {
      return reinterpret_cast<object_state_t *>(m_next & size_inverse(7));
    }
    CGC1_OPT_INLINE void object_state_t::set_next(object_state_t *state) noexcept
    {
      size_t ptr = (size_t)state;
      size_t iv = (size_t)not_available();
      m_next = (ptr & ((size_t)-4)) | (iv & 1);
    }
    CGC1_OPT_INLINE uint8_t *object_state_t::object_start() const noexcept
    {
      return const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(this) + align(sizeof(object_state_t)));
    }
    CGC1_OPT_INLINE size_t object_state_t::object_size() const noexcept
    {
      // It is invariant that object_start() > next for all valid objects.
      return static_cast<size_t>(reinterpret_cast<uint8_t *>(next()) - object_start());
    }
    CGC1_OPT_INLINE uint8_t *object_state_t::object_end() const noexcept
    {
      return const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(this) + align(sizeof(object_state_t)) + object_size());
    }
    CGC1_OPT_INLINE user_data_base_t *object_state_t::user_data() const noexcept
    {
      return reinterpret_cast<user_data_base_t *>(m_user_data & (~((size_t)7)));
    }
    CGC1_OPT_INLINE void object_state_t::set_user_data(void *user_data) noexcept
    {
      assert(user_data == align_pow2(user_data, 3));
      m_user_data = reinterpret_cast<size_t>(user_data) | user_flags();
    }
    CGC1_OPT_INLINE size_t object_state_t::user_flags() const noexcept
    {
      return m_user_data & 7;
    }
    CGC1_OPT_INLINE void object_state_t::set_user_flags(size_t flags) noexcept
    {
      assert(flags < 8);
      m_user_data = reinterpret_cast<size_t>(user_data()) | flags;
    }
  }
}
