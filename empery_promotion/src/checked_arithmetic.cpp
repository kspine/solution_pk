#include "precompiled.hpp"
#include <poseidon/exception.hpp>

namespace EmperyPromotion {

template<typename T>
T checked_add(T lhs, T rhs){
	const T ret = lhs + rhs;
	if(ret < lhs){
		LOG_EMPERY_PROMOTION_ERROR("Integral addition overflow: lhs = ", +lhs, ", rhs = ", +rhs);
		DEBUG_THROW(Exception, sslit("Integral addition overflow"));
	}
	return ret;
}

template std::uint8_t  checked_add<std::uint8_t >(std::uint8_t  lhs, std::uint8_t  rhs);
template std::uint16_t checked_add<std::uint16_t>(std::uint16_t lhs, std::uint16_t rhs);
template std::uint32_t checked_add<std::uint32_t>(std::uint32_t lhs, std::uint32_t rhs);
template std::uint64_t checked_add<std::uint64_t>(std::uint64_t lhs, std::uint64_t rhs);

template<typename T>
T checked_mul(T lhs, T rhs){
	if(lhs == 0){
		return 0;
	}
	const T ret = lhs * rhs;
	if(((lhs | rhs) & -(1ul << sizeof(T) / 2)) && (ret / lhs != rhs)){
		LOG_EMPERY_PROMOTION_ERROR("Integral multiplication overflow: lhs = ", +lhs, ", rhs = ", +rhs);
		DEBUG_THROW(Exception, sslit("Integral addition overflow"));
	}
	return ret;
}

template std::uint8_t  checked_mul<std::uint8_t >(std::uint8_t  lhs, std::uint8_t  rhs);
template std::uint16_t checked_mul<std::uint16_t>(std::uint16_t lhs, std::uint16_t rhs);
template std::uint32_t checked_mul<std::uint32_t>(std::uint32_t lhs, std::uint32_t rhs);
template std::uint64_t checked_mul<std::uint64_t>(std::uint64_t lhs, std::uint64_t rhs);

}
