#pragma once
#include <boost/property_tree/ptree_fwd.hpp>
#include <cgc1/declarations.hpp>
namespace cgc1
{
  class global_kernel_state_param_t
  {
  public:
    global_kernel_state_param_t();
    global_kernel_state_param_t(const global_kernel_state_param_t &) noexcept;
    global_kernel_state_param_t(global_kernel_state_param_t &&) noexcept;
    global_kernel_state_param_t &operator=(const global_kernel_state_param_t &) noexcept;
    global_kernel_state_param_t &operator=(global_kernel_state_param_t &&) noexcept;
    ~global_kernel_state_param_t();
    /**
     * \brief Set size of slab allocator at start.
     **/
    void set_slab_allocator_start_size(size_t sz);
    /**
     * \brief Set expansion size of slab allocator.
     **/
    void set_slab_allocator_expansion_size(size_t sz);
    /**
     * \brief Set size of packed allocator at start.
     **/
    void set_packed_allocator_start_size(size_t sz);
    /**
     * \brief Set expansion size of packed allocator.
     **/
    void set_packed_allocator_expansion_size(size_t sz);
    /**
     * \brief Set size of internal allocator at start.
     **/
    void set_internal_allocator_start_size(size_t sz);
    /**
     * \brief Set expansion size of internal allocator.
     **/
    void set_internal_allocator_expansion_size(size_t sz);
    /**
     * \brief Return size of slab allocator at start.
     **/
    auto slab_allocator_start_size() const noexcept -> size_t;
    /**
     * \brief Return expansion size of slab allocator.
     **/
    auto slab_allocator_expansion_size() const noexcept -> size_t;
    /**
     * \brief Return size of packed allocator at start.
     **/
    auto packed_allocator_start_size() const noexcept -> size_t;
    /**
     * \brief Return expansion size of packed allocator.
     **/
    auto packed_allocator_expansion_size() const noexcept -> size_t;
    /**
     * \brief Return size of internal allocator at start.
     **/
    auto internal_allocator_start_size() const noexcept -> size_t;
    /**
     * \brief Return expansion size of internal allocator.
     **/
    auto internal_allocator_expansion_size() const noexcept -> size_t;
    /**
     * \brief Put settings into a property tree.
     **/
    void to_ptree(::boost::property_tree::ptree &ptree) const;

  private:
    /*
     * \brief Size of slab allocator at start.
     */
    size_t m_slab_allocator_start_size = ::mcpputil::pow2(22);
    /**
     * \brief Expansion size of slab allocator.
     **/
    size_t m_slab_allocator_expansion_size = ::mcpputil::pow2(24);
    /**
     * \brief Size of packed allocator at start.
     **/
    size_t m_packed_allocator_start_size = ::mcpputil::pow2(33);
    /**
     * \brief Expansion size of packed allocator.
     **/
    size_t m_packed_allocator_expansion_size = ::mcpputil::pow2(34);
    /**
     * \brief Size of internal allocator at start.
     **/
    size_t m_internal_allocator_start_size = ::mcpputil::pow2(31);
    /**
     * \brief Expansion size of internal allocator.
     **/
    size_t m_internal_allocator_expansion_size = ::mcpputil::pow2(33);
  };
}
