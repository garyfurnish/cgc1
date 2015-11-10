#include "global_kernel_state_param.hpp"
#include <mcppalloc_utils/boost/property_tree/ptree.hpp>
namespace cgc1
{
  global_kernel_state_param_t::global_kernel_state_param_t() = default;
  global_kernel_state_param_t::global_kernel_state_param_t(const global_kernel_state_param_t &) noexcept = default;
  global_kernel_state_param_t::global_kernel_state_param_t(global_kernel_state_param_t &&) noexcept = default;
  global_kernel_state_param_t &global_kernel_state_param_t::operator=(const global_kernel_state_param_t &) noexcept = default;
  global_kernel_state_param_t &global_kernel_state_param_t::operator=(global_kernel_state_param_t &&) noexcept = default;
  global_kernel_state_param_t::~global_kernel_state_param_t() = default;
  void global_kernel_state_param_t::set_slab_allocator_start_size(size_t sz)
  {
    m_slab_allocator_start_size = sz;
  }
  void global_kernel_state_param_t::set_slab_allocator_expansion_size(size_t sz)
  {
    m_slab_allocator_expansion_size = sz;
  }
  void global_kernel_state_param_t::set_packed_allocator_start_size(size_t sz)
  {
    m_packed_allocator_start_size = sz;
  }
  void global_kernel_state_param_t::set_packed_allocator_expansion_size(size_t sz)
  {
    m_packed_allocator_expansion_size = sz;
  }
  void global_kernel_state_param_t::set_internal_allocator_start_size(size_t sz)
  {
    m_internal_allocator_start_size = sz;
  }
  void global_kernel_state_param_t::set_internal_allocator_expansion_size(size_t sz)
  {
    m_internal_allocator_expansion_size = sz;
  }
  auto global_kernel_state_param_t::slab_allocator_start_size() const noexcept -> size_t
  {
    return m_slab_allocator_start_size;
  }
  auto global_kernel_state_param_t::slab_allocator_expansion_size() const noexcept -> size_t
  {
    return m_slab_allocator_expansion_size;
  }
  auto global_kernel_state_param_t::packed_allocator_start_size() const noexcept -> size_t
  {
    return m_packed_allocator_start_size;
  }
  auto global_kernel_state_param_t::packed_allocator_expansion_size() const noexcept -> size_t
  {
    return m_packed_allocator_expansion_size;
  }
  auto global_kernel_state_param_t::internal_allocator_start_size() const noexcept -> size_t
  {
    return m_internal_allocator_start_size;
  }
  auto global_kernel_state_param_t::internal_allocator_expansion_size() const noexcept -> size_t
  {
    return m_internal_allocator_expansion_size;
  }
  void global_kernel_state_param_t::to_ptree(::boost::property_tree::ptree &ptree) const
  {
    ptree.put("slab_allocator_start_size", ::std::to_string(slab_allocator_start_size()));
    ptree.put("slab_allocator_expansion_size", ::std::to_string(slab_allocator_expansion_size()));
    ptree.put("packed_allocator_start_size", ::std::to_string(packed_allocator_start_size()));
    ptree.put("packed_allocator_expansion_size", ::std::to_string(packed_allocator_expansion_size()));
    ptree.put("internal_allocator_start_size", ::std::to_string(internal_allocator_start_size()));
    ptree.put("internal_allocator_expansion_size", ::std::to_string(internal_allocator_expansion_size()));
  }
}
