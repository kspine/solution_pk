#ifndef EMPERY_GOLD_SCRAMBLE_CHECKED_ARITHMETIC_HPP_
#define EMPERY_GOLD_SCRAMBLE_CHECKED_ARITHMETIC_HPP_

#include <boost/cstdint.hpp>
#include <type_traits>
#include <poseidon/exception.hpp>

namespace EmperyGoldScramble {

template<typename T>
T checkedAdd(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	const T ret = lhs + rhs;
	if(ret < lhs){
		DEBUG_THROW(Poseidon::Exception, Poseidon::sslit("Integral addition overflow"));
	}
	return ret;
}

template boost::uint8_t  checkedAdd<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t checkedAdd<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t checkedAdd<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t checkedAdd<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

template<typename T>
T saturatedAdd(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	const T ret = lhs + rhs;
	if(ret < lhs){
		return static_cast<T>(-1);
	}
	return ret;
}

template boost::uint8_t  saturatedAdd<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t saturatedAdd<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t saturatedAdd<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t saturatedAdd<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

template<typename T>
T checkedSub(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	const T ret = lhs - rhs;
	if(ret > lhs){
		DEBUG_THROW(Poseidon::Exception, Poseidon::sslit("Integral subtraction overflow"));
	}
	return ret;
}

template boost::uint8_t  checkedSub<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t checkedSub<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t checkedSub<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t checkedSub<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

template<typename T>
T saturatedSub(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	const T ret = lhs - rhs;
	if(ret > lhs){
		return 0;
	}
	return ret;
}

template boost::uint8_t  saturatedSub<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t saturatedSub<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t saturatedSub<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t saturatedSub<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

template<typename T>
T checkedMul(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	if((lhs == 0) || (rhs == 0)){
		return 0;
	}
	const T ret = lhs * rhs;
	if(ret / lhs != rhs){
		DEBUG_THROW(Poseidon::Exception, Poseidon::sslit("Integral multiplication over flow"));
	}
	return ret;
}

template boost::uint8_t  checkedMul<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t checkedMul<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t checkedMul<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t checkedMul<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

template<typename T>
T saturatedMul(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	if((lhs == 0) || (rhs == 0)){
		return 0;
	}
	const T ret = lhs * rhs;
	if(ret / lhs != rhs){
		return static_cast<T>(-1);
	}
	return ret;
}

template boost::uint8_t  saturatedMul<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t saturatedMul<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t saturatedMul<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t saturatedMul<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

}

#endif
