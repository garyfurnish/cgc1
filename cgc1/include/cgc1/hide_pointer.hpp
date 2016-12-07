#include <cstdint>
#include <mcpputil/mcpputil/declarations.hpp>
namespace mcpputil
{
  /**
   * \brief Hide a pointer from garbage collection in a unspecified way.
   **/
  mcpputil_always_inline uintptr_t hide_pointer(void *v)
  {
    return ~reinterpret_cast<size_t>(v);
  }
  /**
   * \brief Unide a pointer from garbage collection in a unspecified way.
   **/
  mcpputil_always_inline void *unhide_pointer(uintptr_t sz)
  {
    return reinterpret_cast<void *>(~sz);
  }
}
