#ifndef EMPERY_CENTER_ID_TYPES_HPP_
#define EMPERY_CENTER_ID_TYPES_HPP_

#include <iosfwd>
#include <boost/cstdint.hpp>
#include <poseidon/cxx_ver.hpp>
#include <poseidon/uuid.hpp>

namespace EmperyCenter {

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
		return *this + 1;
	}
	GenericId operator--(int){
		return *this - 1;
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

template<int>
class GenericUuid {
private:
	Poseidon::Uuid m_uuid;

public:
	explicit constexpr GenericUuid()
		: m_uuid()
	{
	}

public:
	constexpr const Poseidon::Uuid &get() const {
		return m_uuid;
	}
	void set(const Poseidon::Uuid &uuid){
		m_uuid = uuid;
	}

public:
	constexpr bool operator==(const GenericUuid &rhs) const {
		return get() == rhs.get();
	}
	constexpr bool operator!=(const GenericUuid &rhs) const {
		return get() != rhs.get();
	}
	constexpr bool operator<(const GenericUuid &rhs) const {
		return get() < rhs.get();
	}
	constexpr bool operator>(const GenericUuid &rhs) const {
		return get() > rhs.get();
	}
	constexpr bool operator<=(const GenericUuid &rhs) const {
		return get() <= rhs.get();
	}
	constexpr bool operator>=(const GenericUuid &rhs) const {
		return get() >= rhs.get();
	}

	constexpr explicit operator bool() const noexcept {
		return get() != Poseidon::Uuid();
	}
};

template<int MAGIC_T>
std::ostream &operator<<(std::ostream &os, const GenericUuid<MAGIC_T> &id){
	os <<id.get();
	return os;
}
template<int MAGIC_T>
std::istream &operator>>(std::istream &is, GenericUuid<MAGIC_T> &id){
	Poseidon::Uuid tmp;
	if(is >>tmp){
		id.set(tmp);
	}
	return is;
}

using TerrainId         = GenericId<boost::uint32_t, 110001>;
using OverlayId         = GenericId<boost::uint32_t, 110002>;
using ResourceId        = GenericId<boost::uint32_t, 110003>

using AccountUuid       = GenericUuid<21001>;
using CastleUuid        = GenericUuid<21002>;
using UnitUuid          = GenericUuid<21003>;

}

#endif
