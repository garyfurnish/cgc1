#include "global_kernel_state.hpp"
#include <mcpputil/mcpputil/boost/property_tree/json_parser.hpp>
#include <mcpputil/mcpputil/boost/property_tree/ptree.hpp>
namespace cgc1
{
  void to_ptree(const details::global_kernel_state_t &state, ::boost::property_tree::ptree &ptree, int level)
  {
    {
      ::boost::property_tree::ptree init;
      state.initialization_parameters_ref().to_ptree(init);
      ptree.put_child("initialization", init);
    }
    {
      ::boost::property_tree::ptree last_collect;
      last_collect.put("clear_mark_time", ::std::to_string(state.clear_mark_time_span().count()));
      last_collect.put("mark_time", ::std::to_string(state.mark_time_span().count()));
      last_collect.put("sweep_time", ::std::to_string(state.sweep_time_span().count()));
      last_collect.put("notify_time", ::std::to_string(state.notify_time_span().count()));
      last_collect.put("total_time", ::std::to_string(state.total_collect_time_span().count()));
      ptree.put_child("last_collect", last_collect);
    }
    {
      ::boost::property_tree::ptree slab_allocator;
      state._internal_slab_allocator().to_ptree(slab_allocator, level);
      ptree.put_child("slab_allocator", slab_allocator);
    }
    {
      ::boost::property_tree::ptree cgc_allocator;
      state._internal_allocator().to_ptree(cgc_allocator, level);
      ptree.put_child("cgc_allocator", cgc_allocator);
    }
    {
      ::boost::property_tree::ptree gc_allocator;
      state.gc_allocator().to_ptree(gc_allocator, level);
      ptree.put_child("gc_allocator", gc_allocator);
    }
    {
      ::boost::property_tree::ptree packed_allocator;
      state._bitmap_allocator().to_ptree(packed_allocator, level);
      ptree.put_child("packed_allocator", packed_allocator);
    }
  }
  auto to_json(const details::global_kernel_state_t &state, int level) -> ::std::string
  {
    ::std::stringstream ss;
    ::boost::property_tree::ptree ptree;
    to_ptree(state, ptree, level);
    ::boost::property_tree::json_parser::write_json(ss, ptree);
    return ss.str();
  }
}
