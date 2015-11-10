#pragma once
namespace mcppalloc
{
  /**
   * \brief Forward iterator for type T that takes a functional that defines how to advance the iterator.
   *
   * The rule is that Current = Advance(Current).
  **/
  template <typename T, typename Advance>
  class functional_iterator_t
  {
  public:
    template <typename In_Advance>
    /**
     * \brief Constructor.
     * @param ptr Start object.
     * @param advance Functional used to advance to next object.
     **/
    functional_iterator_t(T *t, In_Advance &&advance) noexcept : m_t(t), m_advance(::std::forward<In_Advance>(advance))
    {
    }
    functional_iterator_t(const functional_iterator_t<T, Advance> &) noexcept = default;
    functional_iterator_t(functional_iterator_t<T, Advance> &&) noexcept = default;
    functional_iterator_t<T, Advance> &operator=(const functional_iterator_t &) noexcept = default;
    functional_iterator_t<T, Advance> &operator=(functional_iterator_t &&) noexcept = default;
    T &operator*() const noexcept
    {
      return *m_t;
    }
    T *operator->() const noexcept
    {
      return m_t;
    }
    operator T *() const noexcept
    {
      return m_t;
    }
    functional_iterator_t<T, Advance> &operator++(int) noexcept
    {
      auto ret = functional_iterator_t<T, Advance>(m_t, m_advance);
      m_t = m_advance(m_t);
      return ret;
    }
    functional_iterator_t<T, Advance> &operator++() noexcept
    {
      m_t = m_advance(m_t);
      return *this;
    }
    bool operator==(const functional_iterator_t<T, Advance> &it) const noexcept
    {
      return m_t == it.m_t;
    }
    bool operator!=(const functional_iterator_t<T, Advance> &it) const noexcept
    {
      return m_t != it.m_t;
    }

  public:
    T *m_t;
    Advance m_advance;
  };
  /**
   * \brief Function to make functional iterators that uses type deduction to avoid end user templates.
  **/
  template <typename T, typename Advance>
  auto make_functional_iterator(T *t, Advance &&advance) noexcept -> functional_iterator_t<T, Advance>
  {
    return functional_iterator_t<T, Advance>(t, ::std::forward<Advance>(advance));
  }
  /**
   * \brief Functional that gets the next element from a type.
   **/
  struct iterator_next_advancer_t {
    template <typename T>
    auto operator()(T &&t) const noexcept -> decltype(t->next())
    {
      return ::std::forward<T>(t)->next();
    }
  };
  /**
   * \brief Iterator that works by obtaining the next element.
   **/
  template <typename T>
  using next_iterator = functional_iterator_t<T, iterator_next_advancer_t>;
  /**
   * \brief Function to make next iterators that uses type deduction to avoid end user templates.
  **/
  template <typename T>
  auto make_next_iterator(T *t) noexcept -> next_iterator<T>
  {
    return make_functional_iterator(t, iterator_next_advancer_t{});
  }
}
