#include <mcppalloc_utils/boost/property_tree/ptree.hpp>
#include <mcppalloc_utils/boost/property_tree/json_parser.hpp>
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
