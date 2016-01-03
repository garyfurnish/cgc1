#pragma once
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {
      /**
       * \brief Class that stores allocation information about an allocation type.
       **/
      class bitmap_type_info_t
      {
      public:
        bitmap_type_info_t() = default;
        /**
         * \brief Constructor.
         * @param type_id Type id.
         * @param num_user_bits Number of user bit fields for this allocation type.
         **/
        bitmap_type_info_t(type_id_t type_id, uint8_t num_user_bits);
        /**
         * \brief Return allocation type id.
         **/
        auto type_id() const noexcept -> type_id_t;
        /**
         * \brief Set allocation type id.
         **/
        void set_type_id(type_id_t type_id) noexcept;
        /**
         * \brief Return number of user bit fields.
         **/
        auto num_user_bits() const noexcept -> uint8_t;
        /**
         * \brief Set number of user bit fields.
         **/
        void set_num_user_bits(uint8_t num_user_bits) noexcept;

      private:
        type_id_t m_type_id{::std::numeric_limits<type_id_t>::max()};
        uint8_t m_num_user_bits{0};
      };
    }
  }
}
#include "bitmap_type_info_impl.hpp"
