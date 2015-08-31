#include "packed_object_thread_allocator.hpp"
namespace cgc1
{
  namespace details
  {
    void packed_object_package_t::insert(const packed_object_package_t &state)
    {
      m_32.insert(m_32.begin(), state.m_32.begin(), state.m_32.end());
      m_64.insert(m_64.begin(), state.m_64.begin(), state.m_64.end());
      m_128.insert(m_128.begin(), state.m_128.begin(), state.m_128.end());
      m_256.insert(m_256.begin(), state.m_256.begin(), state.m_256.end());
      m_512.insert(m_512.begin(), state.m_512.begin(), state.m_512.end());
    }
  }
}
