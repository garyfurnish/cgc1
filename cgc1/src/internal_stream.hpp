#include <mcppalloc/object_state.hpp>
#include <iostream>
namespace cgc1
{
  namespace details
  {
    template <typename charT, typename Traits, typename Policy>
    ::std::basic_ostream<charT, Traits> &operator<<(::std::basic_ostream<charT, Traits> &os,
                                                    const ::mcppalloc::details::object_state_t<Policy> &state)
    {
      os << "Object_State_t = (" << &state << "," << state.next() << "," << state.not_available() << "," << state.m_user_data
         << ")";
      return os;
    }
  }
}
