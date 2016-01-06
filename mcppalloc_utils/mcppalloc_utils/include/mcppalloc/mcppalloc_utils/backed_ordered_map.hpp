#pragma once
#include "declarations.hpp"
#include <mcppalloc/mcppalloc_utils/alignment.hpp>
#include <stdexcept>
namespace mcppalloc
{
  namespace containers
  {
    template <typename K, typename V, typename Less = ::std::less<K>>
    class backed_ordered_map
    {
    public:
      using backing_pointer_type = void *;
      using key_type = K;
      using mapped_type = V;
      using value_type = ::std::pair<key_type, mapped_type>;
      using key_compare = Less;
      using iterator = value_type *;
      using const_iterator = const value_type *;
      using size_type = size_t;
      using difference_type = ptrdiff_t;

      backed_ordered_map();
      template <typename KC = key_compare>
      backed_ordered_map(backing_pointer_type v, size_t capacity, KC &&kc = key_compare());
      backed_ordered_map(const backed_ordered_map &) = delete;
      backed_ordered_map(backed_ordered_map &&);
      backed_ordered_map &operator=(const backed_ordered_map &) = delete;
      backed_ordered_map &operator=(backed_ordered_map &&);
      ~backed_ordered_map();

      auto size() noexcept -> size_type;
      auto capacity() noexcept -> size_type;
      auto data() noexcept -> value_type *;
      auto data() const noexcept -> const value_type *;
      /**
       * \brief Returns an iterator to the first element contained in the container.
       *
       * Throws: Nothing.
       * Complexity: Constant.
       **/
      auto begin() noexcept -> iterator;
      /**
       * \brief Returns an iterator to the first element contained in the container.
       *
       * Throws: Nothing.
       * Complexity: Constant.
       **/
      auto begin() const noexcept -> const_iterator;
      /**
       * \brief Returns an iterator to the first element contained in the container.
       *
       * Throws: Nothing.
       * Complexity: Constant.
       **/
      auto cbegin() const noexcept -> const_iterator;
      /**
       * \brief Returns an iterator to the end of the container.
       *
       * Throws: Nothing.
       * Complexity: Constant.
       **/
      auto end() noexcept -> iterator;
      /**
       * \brief Returns an iterator to the end of the container.
       *
       * Throws: Nothing.
       * Complexity: Constant.
       **/
      auto end() const noexcept -> const_iterator;
      /**
       * \brief Returns an iterator to the end of the container.
       *
       * Throws: Nothing.
       * Complexity: Constant.
       **/
      auto cend() const noexcept -> const_iterator;
      auto find(const key_type &k) -> iterator;
      auto find(const key_type &k) const -> const_iterator;

      auto operator[](const key_type &) -> mapped_type &;
      auto operator[](key_type &&) -> mapped_type &;

      auto at(const key_type &) -> mapped_type &;
      auto at(const key_type &) const -> const mapped_type &;

      /**
       * \brief Inserts a new value_type move constructed from the pair if and only if there is no element in the container with
       *key equivalent to the key of x.
       *
       * Complexity: Logarithmic search time plus linear insertion to the elements with bigger keys than x.
       * Note: If an element it's inserted it might invalidate elements.
       * @return The bool component of the returned pair is true if and only if the insertion takes place, and the iterator
       *component of the pair points to the element with key equivalent to the key of x.
       **/
      auto insert(const value_type &) -> std::pair<iterator, bool>;
      /**
       * \brief Inserts a new value_type move constructed from the pair if and only if there is no element in the container with
       *key equivalent to the key of x.
       *
       * Complexity: Logarithmic search time plus linear insertion to the elements with bigger keys than x.
       * Note: If an element it's inserted it might invalidate elements.
       * @return The bool component of the returned pair is true if and only if the insertion takes place, and the iterator
       *component of the pair points to the element with key equivalent to the key of x.
       **/
      auto insert(value_type &&) -> ::std::pair<iterator, bool>;
      /**
       * \brief Erases all elements in the container with key equivalent to x.
       *
       * Complexity: Logarithmic search time plus erasure time linear to the elements with bigger keys.
       * @return Returns the number of erased elements.
      **/
      auto erase(const key_type &) -> size_type;
      /**
       * \brief erase(a.begin(),a.end()).
       *
       * Postcondition: size() == 0.
       * Complexity: linear in size().
      **/
      void clear();
      /**
       *
       * Complexity: Logarithmic
       **/
      auto lower_bound(const key_type &) -> iterator;
      /**
       *
       * Complexity: Logarithmic
       **/
      auto lower_bound(const key_type &) const -> const_iterator;
      /**
       *
       * Complexity: Logarithmic
       **/
      auto upper_bound(const key_type &) -> iterator;
      /**
       *
       * Complexity: Logarithmic
       **/
      auto upper_bound(const key_type &) const -> const_iterator;
      /**
       * \brief Remove key and insert value.
       *
       * This can be done faster in many cases.
       * This does not throw but returns false if k does not exist.
       * @return Iterator and true if move took place, false otherwise.
       **/
      auto move(const key_type &k, value_type &&v) -> ::std::pair<iterator, bool>;

    private:
      void *m_v{nullptr};
      size_t m_size{0};
      size_t m_capacity{0};
      key_compare m_compare;
    };
  }
}
#include "backed_ordered_map_impl.hpp"
