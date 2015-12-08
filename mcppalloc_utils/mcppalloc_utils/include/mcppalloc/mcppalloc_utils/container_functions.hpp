#pragma once
namespace mcppalloc
{
  template <typename Begin, typename End, typename Val, typename Comparator>
  auto last_greater_equal_than(Begin &&begin, End &&end, Val &&val, Comparator &&comparator)
  {
    if (begin == end)
      return end;
    const auto ub = ::std::upper_bound(::std::forward<Begin>(begin), ::std::forward<End>(end), ::std::forward<Val>(val),
                                       ::std::forward<Comparator>(comparator));
    const auto plb = ub - 1;
    if (ub == begin || comparator(*plb, val)) {
      return ub;
    }
    else {
      return plb;
    }
  }
  template <typename T>
  void clear_capacity(T &&t)
  {
    t.clear();
    t.shrink_to_fit();
    auto a = ::std::move(t);
  }
  template <typename Iterator, typename Val>
  void insert_replace(Iterator replace, Iterator new_location, Val &&val)
  {
    assert(replace <= new_location);
    ::std::rotate(replace, replace + 1, new_location + 1);
    *new_location = ::std::forward<Val>(val);
  }
  /**
   * \brief Insert val into a sorted container if val would be unique.
  **/
  template <typename Container, typename T, typename Compare>
  bool insert_unique_sorted(Container &c, const T &val, Compare comp)
  {
    // if empty container, just add it!
    if (c.empty()) {
      c.push_back(val);
      return true;
    }
    // otherwise find an iterator to insert before.
    // TODO: THIS deserves further testing.
    // Changing this code doesn't seem to do anything.
    auto lb = ::std::lower_bound(c.begin(), c.end(), val, comp);
    auto origlb = lb;
    while (lb != c.end() && comp(*lb, val)) {
      if (*lb == val)
        return false;
      lb++;
    }
    // insert before lb.
    c.insert(origlb, val);
    return true;
  }
}
