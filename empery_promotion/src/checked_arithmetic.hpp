#ifndef EMPERY_PROMOTION_CHECKED_ARITHMETIC_HPP_
#define EMPERY_PROMOTION_CHECKED_ARITHMETIC_HPP_

#include <boost/cstdint.hpp>

namespace EmperyPromotion {

template<typename T>
T checked_add(T lhs, T rhs);

extern template boost::uint8_t  checked_add<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
extern template boost::uint16_t checked_add<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
extern template boost::uint32_t checked_add<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
extern template boost::uint64_t checked_add<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

template<typename T>
T checked_mul(T lhs, T rhs);

extern template boost::uint8_t  checked_mul<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
extern template boost::uint16_t checked_mul<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
extern template boost::uint32_t checked_mul<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
extern template boost::uint64_t checked_mul<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

}

#endif
