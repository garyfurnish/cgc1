#pragma once
namespace cgc1
{
  namespace details
  {
    /**
   * \brief User data that can be associated with an allocation.
   **/
    class gc_user_data_t : public user_data_base_t
    {
    public:
      /**
       * \brief Constructor
       *
       * @param block Block that this allocation belongs to.
       **/
      gc_user_data_t() = default;
      /**
       * \brief Optional finalizer function to run.
      **/
      ::std::function<void(void *)> m_finalizer = nullptr;

    private:
      /**
       * \brief True if uncollectable, false otherwise.
       *
       * Only used for gc_type.
      **/
      bool m_uncollectable = false;
    };
  }
}
