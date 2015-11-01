#include "object_state.hpp"
#include <iostream>
namespace cgc1
{
  namespace details
  {
    template <typename charT, typename Traits>
    ::std::basic_ostream<charT, Traits> &operator<<(::std::basic_ostream<charT, Traits> &os, const object_state_t &state)
    {
      os << "Object_State_t = (" << &state << "," << state.next() << "," << state.not_available() << "," << state.m_user_data
         << ")";
      return os;
    }
  }
}
