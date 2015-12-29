#ifndef EMPERY_GOLD_SCRAMBLE_CHECKED_ARITHMETIC_HPP_
#define EMPERY_GOLD_SCRAMBLE_CHECKED_ARITHMETIC_HPP_

#include <cstdint>
#include <type_traits>
#include <poseidon/exception.hpp>

namespace EmperyGoldScramble {

template<typename T>
T checked_add(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	const T ret = lhs + rhs;
	if(ret < lhs){
		DEBUG_THROW(Poseidon::Exception, Poseidon::sslit("Integral addition overflow"));
	}
	return ret;
}

template std::uint8_t  checked_add<std::uint8_t >(std::uint8_t  lhs, std::uint8_t  rhs);
template std::uint16_t checked_add<std::uint16_t>(std::uint16_t lhs, std::uint16_t rhs);
template std::uint32_t checked_add<std::uint32_t>(std::uint32_t lhs, std::uint32_t rhs);
template std::uint64_t checked_add<std::uint64_t>(std::uint64_t lhs, std::uint64_t rhs);

template<typename T>
T saturated_add(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	const T ret = lhs + rhs;
	if(ret < lhs){
		return static_cast<T>(-1);
	}
	return ret;
}

template std::uint8_t  saturated_add<std::uint8_t >(std::uint8_t  lhs, std::uint8_t  rhs);
template std::uint16_t saturated_add<std::uint16_t>(std::uint16_t lhs, std::uint16_t rhs);
template std::uint32_t saturated_add<std::uint32_t>(std::uint32_t lhs, std::uint32_t rhs);
template std::uint64_t saturated_add<std::uint64_t>(std::uint64_t lhs, std::uint64_t rhs);

template<typename T>
T checked_sub(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	const T ret = lhs - rhs;
	if(ret > lhs){
		DEBUG_THROW(Poseidon::Exception, Poseidon::sslit("Integral subtraction overflow"));
	}
	return ret;
}

template std::uint8_t  checked_sub<std::uint8_t >(std::uint8_t  lhs, std::uint8_t  rhs);
template std::uint16_t checked_sub<std::uint16_t>(std::uint16_t lhs, std::uint16_t rhs);
template std::uint32_t checked_sub<std::uint32_t>(std::uint32_t lhs, std::uint32_t rhs);
template std::uint64_t checked_sub<std::uint64_t>(std::uint64_t lhs, std::uint64_t rhs);

template<typename T>
T saturated_sub(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	const T ret = lhs - rhs;
	if(ret > lhs){
		return 0;
	}
	return ret;
}

template std::uint8_t  saturated_sub<std::uint8_t >(std::uint8_t  lhs, std::uint8_t  rhs);
template std::uint16_t saturated_sub<std::uint16_t>(std::uint16_t lhs, std::uint16_t rhs);
template std::uint32_t saturated_sub<std::uint32_t>(std::uint32_t lhs, std::uint32_t rhs);
template std::uint64_t saturated_sub<std::uint64_t>(std::uint64_t lhs, std::uint64_t rhs);

template<typename T>
T checked_mul(T lhs, T rhs){
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

template std::uint8_t  checked_mul<std::uint8_t >(std::uint8_t  lhs, std::uint8_t  rhs);
template std::uint16_t checked_mul<std::uint16_t>(std::uint16_t lhs, std::uint16_t rhs);
template std::uint32_t checked_mul<std::uint32_t>(std::uint32_t lhs, std::uint32_t rhs);
template std::uint64_t checked_mul<std::uint64_t>(std::uint64_t lhs, std::uint64_t rhs);

template<typename T>
T saturated_mul(T lhs, T rhs){
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

template std::uint8_t  saturated_mul<std::uint8_t >(std::uint8_t  lhs, std::uint8_t  rhs);
template std::uint16_t saturated_mul<std::uint16_t>(std::uint16_t lhs, std::uint16_t rhs);
template std::uint32_t saturated_mul<std::uint32_t>(std::uint32_t lhs, std::uint32_t rhs);
template std::uint64_t saturated_mul<std::uint64_t>(std::uint64_t lhs, std::uint64_t rhs);

}

#endif
