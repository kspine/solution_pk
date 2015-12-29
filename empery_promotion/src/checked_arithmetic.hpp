#ifndef EMPERY_PROMOTION_CHECKED_ARITHMETIC_HPP_
#define EMPERY_PROMOTION_CHECKED_ARITHMETIC_HPP_

#include <cstdint>

namespace EmperyPromotion {

template<typename T>
T checked_add(T lhs, T rhs);

extern template std::uint8_t  checked_add<std::uint8_t >(std::uint8_t  lhs, std::uint8_t  rhs);
extern template std::uint16_t checked_add<std::uint16_t>(std::uint16_t lhs, std::uint16_t rhs);
extern template std::uint32_t checked_add<std::uint32_t>(std::uint32_t lhs, std::uint32_t rhs);
extern template std::uint64_t checked_add<std::uint64_t>(std::uint64_t lhs, std::uint64_t rhs);

template<typename T>
T checked_mul(T lhs, T rhs);

extern template std::uint8_t  checked_mul<std::uint8_t >(std::uint8_t  lhs, std::uint8_t  rhs);
extern template std::uint16_t checked_mul<std::uint16_t>(std::uint16_t lhs, std::uint16_t rhs);
extern template std::uint32_t checked_mul<std::uint32_t>(std::uint32_t lhs, std::uint32_t rhs);
extern template std::uint64_t checked_mul<std::uint64_t>(std::uint64_t lhs, std::uint64_t rhs);

}

#endif
