#include "precompiled.hpp"
#include "checked_arithmetic.hpp"
#include <poseidon/exception.hpp>

namespace EmperyPromotion {

template<typename T>
T checkedAdd(T lhs, T rhs){
	const T ret = lhs + rhs;
	if(ret < lhs){
		LOG_EMPERY_PROMOTION_ERROR("Integral addition overflow: lhs = ", +lhs, ", rhs = ", +rhs);
		DEBUG_THROW(Exception, sslit("Integral addition overflow"));
	}
	return ret;
}

template boost::uint8_t  checkedAdd<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t checkedAdd<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t checkedAdd<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t checkedAdd<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

template<typename T>
T checkedMul(T lhs, T rhs){
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

template boost::uint8_t  checkedMul<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t checkedMul<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t checkedMul<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t checkedMul<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

}
