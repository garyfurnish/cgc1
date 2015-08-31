#pragma once
namespace cgc1
{
  namespace details
  {
    static inline size_t get_packed_object_size_id(size_t sz)
    {
      const size_t min_2 = 5;
      const size_t max_bins = 4;
      sz -= 1;
      sz = sz >> min_2;
      if (!sz)
        return 0;
      size_t ret = 64 - static_cast<size_t>(__builtin_clzll(sz));
      if (ret > max_bins)
        return ::std::numeric_limits<size_t>::max();
      return ret;
    }
  }
}
