#pragma once
#include <cgc1/declarations.hpp>
namespace cgc1
{
  /**
   * \brief Put information about global kernel state into a property tree.
   * @param level Level of information to give.  Higher is more verbose.
   **/
  void to_ptree(const details::global_kernel_state_t &state, ::boost::property_tree::ptree &ptree, int level);
  /**
   * \brief Put information about global kernel state into a json string.
   * @param level Level of information to give.  Higher is more verbose.
   **/
  auto to_json(const details::global_kernel_state_t &state, int level) -> ::std::string;
}
