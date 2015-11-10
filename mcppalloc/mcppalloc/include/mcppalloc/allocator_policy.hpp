#pragma once
namespace mcppalloc
{
  /**
   * \brief All policies describing behavior of sparse allocator derive from this.
   * For the idea of a policy see:
   * https://stackoverflow.com/questions/14718055/what-is-the-difference-between-a-trait-and-a-policy
   **/
  struct allocator_policy_tag_t {
  public:
  };
}
