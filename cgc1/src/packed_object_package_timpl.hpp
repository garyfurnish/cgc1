#pragma once
namespace cgc1
{
  namespace details
  {
    template <typename Predicate>
    void packed_object_package_t::for_all(Predicate &&predicate)
    {
      for (auto &&vec : m_vectors) {
        for (auto &&state : vec) {
          predicate(state);
        }
      }
    }
  }
}
