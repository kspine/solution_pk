#ifndef EMPERY_PROMOTION_ID_TYPES_HPP_
#define EMPERY_PROMOTION_ID_TYPES_HPP_

#include <iosfwd>
#include <cstdint>
#include <poseidon/cxx_ver.hpp>

namespace EmperyPromotion {

template<typename UnderlyingT, int>
class GenericId {
private:
	UnderlyingT m_id;

public:
	explicit constexpr GenericId(UnderlyingT id = 0)
		: m_id(id)
	{
	}

public:
	constexpr UnderlyingT get() const {
		return m_id;
	}
	void set(UnderlyingT id){
		m_id = id;
	}

public:
	constexpr bool operator==(const GenericId &rhs) const {
		return get() == rhs.get();
	}
	constexpr bool operator!=(const GenericId &rhs) const {
		return get() != rhs.get();
	}
	constexpr bool operator<(const GenericId &rhs) const {
		return get() < rhs.get();
	}
	constexpr bool operator>(const GenericId &rhs) const {
		return get() > rhs.get();
	}
	constexpr bool operator<=(const GenericId &rhs) const {
		return get() <= rhs.get();
	}
	constexpr bool operator>=(const GenericId &rhs) const {
		return get() >= rhs.get();
	}

	constexpr explicit operator bool() const noexcept {
		return get() != 0;
	}

	GenericId &operator+=(UnderlyingT rhs){
		m_id += rhs;
		return *this;
	}
	GenericId &operator-=(UnderlyingT rhs){
		m_id -= rhs;
		return *this;
	}

	GenericId operator+(UnderlyingT rhs) const {
		return GenericId(m_id + rhs);
	}
	GenericId operator-(UnderlyingT rhs) const {
		return GenericId(m_id - rhs);
	}
	UnderlyingT operator-(GenericId rhs) const {
		return m_id - rhs.m_id;
	}

	GenericId &operator++(){
		return *this += 1;
	}
	GenericId &operator--(){
		return *this -= 1;
	}
	GenericId operator++(int){
		auto ret = *this;
		++*this;
		return ret;
	}
	GenericId operator--(int){
		auto ret = *this;
		--*this;
		return ret;
	}
};

template<typename UnderlyingT, int MAGIC_T>
GenericId<UnderlyingT, MAGIC_T> operator+(UnderlyingT lhs, GenericId<UnderlyingT, MAGIC_T> rhs){
	return rhs += lhs;
}

template<typename UnderlyingT, int MAGIC_T>
std::ostream &operator<<(std::ostream &os, const GenericId<UnderlyingT, MAGIC_T> &id){
	os <<id.get();
	return os;
}
template<typename UnderlyingT, int MAGIC_T>
std::istream &operator>>(std::istream &is, GenericId<UnderlyingT, MAGIC_T> &id){
	UnderlyingT tmp;
	if(is >>tmp){
		id.set(tmp);
	}
	return is;
}

using ItemId        = GenericId<std::uint32_t, 10000>;

using AccountId     = GenericId<std::uint64_t, 20000>;

}

#endif
