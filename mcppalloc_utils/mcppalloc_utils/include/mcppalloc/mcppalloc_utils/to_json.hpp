#include <mcppalloc/mcppalloc_utils/boost/property_tree/json_parser.hpp>
#include <mcppalloc/mcppalloc_utils/boost/property_tree/ptree.hpp>
namespace cgc1
{
  template <typename Allocator>
  auto to_json(Allocator &&allocator, int level) -> ::std::string
  {
    ::std::stringstream ss;
    ::boost::property_tree::ptree ptree;
    allocator.to_ptree(ptree, level);
    ::boost::property_tree::json_parser::write_json(ss, ptree);
    return ss.str();
  }
}
