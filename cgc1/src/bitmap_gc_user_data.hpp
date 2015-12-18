#pragma once
#include "gc_user_data.hpp"
namespace cgc1
{
  namespace details
  {
    /**
   * \brief User data that can be associated with an allocation.
   **/
    class bitmap_gc_user_data_t
    {
    public:
      gc_user_data_t &gc_user_data();
      gc_user_data_t &gc_user_data() const;

      bool m_is_atomic;

    private:
      gc_user_data_t m_gc_user_data;
      bool m_atomic;
    };
  }
}
