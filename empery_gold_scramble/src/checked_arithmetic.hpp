#ifndef EMPERY_GOLD_SCRAMBLE_CHECKED_ARITHMETIC_HPP_
#define EMPERY_GOLD_SCRAMBLE_CHECKED_ARITHMETIC_HPP_

#include <boost/cstdint.hpp>
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

template boost::uint8_t  checked_add<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t checked_add<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t checked_add<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t checked_add<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

template<typename T>
T saturated_add(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	const T ret = lhs + rhs;
	if(ret < lhs){
		return static_cast<T>(-1);
	}
	return ret;
}

template boost::uint8_t  saturated_add<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t saturated_add<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t saturated_add<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t saturated_add<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

template<typename T>
T checked_sub(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	const T ret = lhs - rhs;
	if(ret > lhs){
		DEBUG_THROW(Poseidon::Exception, Poseidon::sslit("Integral subtraction overflow"));
	}
	return ret;
}

template boost::uint8_t  checked_sub<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t checked_sub<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t checked_sub<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t checked_sub<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

template<typename T>
T saturated_sub(T lhs, T rhs){
	static_assert(std::is_unsigned<T>::value && !std::is_same<T, bool>::value, "Invalid template type parameter T.");

	const T ret = lhs - rhs;
	if(ret > lhs){
		return 0;
	}
	return ret;
}

template boost::uint8_t  saturated_sub<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t saturated_sub<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t saturated_sub<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t saturated_sub<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

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

template boost::uint8_t  checked_mul<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t checked_mul<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t checked_mul<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t checked_mul<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

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

template boost::uint8_t  saturated_mul<boost::uint8_t >(boost::uint8_t  lhs, boost::uint8_t  rhs);
template boost::uint16_t saturated_mul<boost::uint16_t>(boost::uint16_t lhs, boost::uint16_t rhs);
template boost::uint32_t saturated_mul<boost::uint32_t>(boost::uint32_t lhs, boost::uint32_t rhs);
template boost::uint64_t saturated_mul<boost::uint64_t>(boost::uint64_t lhs, boost::uint64_t rhs);

}

#endif
