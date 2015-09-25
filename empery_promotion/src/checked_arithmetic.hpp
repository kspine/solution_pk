#ifndef EMPERY_PROMOTION_CHECKED_ARITHMETIC_HPP_
#define EMPERY_PROMOTION_CHECKED_ARITHMETIC_HPP_

#include <boost/cstdint.hpp>

namespace EmperyPromotion {

template<typename T>
T checkedAdd(T lhs, T rhs);

extern template boost::uint8_t  checkedAdd<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
extern template boost::uint16_t checkedAdd<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
extern template boost::uint32_t checkedAdd<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
extern template boost::uint64_t checkedAdd<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

template<typename T>
T checkedMul(T lhs, T rhs);

extern template boost::uint8_t  checkedMul<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
extern template boost::uint16_t checkedMul<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
extern template boost::uint32_t checkedMul<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
extern template boost::uint64_t checkedMul<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

}

#endif
