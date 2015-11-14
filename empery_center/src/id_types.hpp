#ifndef EMPERY_CENTER_ID_TYPES_HPP_
#define EMPERY_CENTER_ID_TYPES_HPP_

#include <iosfwd>
#include <iomanip>
#include <boost/cstdint.hpp>
#include <poseidon/cxx_ver.hpp>
#include <poseidon/uuid.hpp>

namespace EmperyCenter {

template<typename UnderlyingT, int>
class GenericId {
private:
	UnderlyingT m_id;

public:
	constexpr GenericId()
		: m_id()
	{
	}
	explicit constexpr GenericId(UnderlyingT id)
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
	UnderlyingT temp;
	if(is >>temp){
		id.set(temp);
	}
	return is;
}

template<int>
class GenericUuid {
private:
	Poseidon::Uuid m_uuid;

public:
	constexpr GenericUuid()
		: m_uuid()
	{
	}
	explicit constexpr GenericUuid(const Poseidon::Uuid &uuid)
		: m_uuid(uuid)
	{
	}
	GenericUuid(const std::string &str)
		: m_uuid(str)
	{
	}

public:
	constexpr const Poseidon::Uuid &get() const {
		return m_uuid;
	}
	void set(const Poseidon::Uuid &uuid){
		m_uuid = uuid;
	}

	std::string str() const {
		return m_uuid.to_string();
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
	char str[37];
	id.get().to_string(reinterpret_cast<char (&)[36]>(str));
	str[36] = 0;
	os <<str;
	return os;
}
template<int MAGIC_T>
std::istream &operator>>(std::istream &is, GenericUuid<MAGIC_T> &id){
	char str[37];
	if(is >>std::setw(sizeof(str)) >>str){
		id.from_string(reinterpret_cast<const char (&)[36]>(str));
	}
	return is;
}

namespace IdTypes {
	using PlatformId        = GenericId<boost::uint32_t, 110001>;
	using LanguageId        = GenericId<boost::uint32_t, 110002>;
	using ReasonId          = GenericId<boost::uint32_t, 110003>;

	using TerrainId         = GenericId<boost::uint32_t, 120001>;
	using OverlayId         = GenericId<boost::uint32_t, 120002>;
	using ResourceId        = GenericId<boost::uint32_t, 120003>;
	using MapObjectTypeId   = GenericId<boost::uint32_t, 120004>;
	using AttributeId       = GenericId<boost::uint32_t, 120005>;
	using BuildingBaseId    = GenericId<boost::uint32_t, 120006>;
	using BuildingId        = GenericId<boost::uint32_t, 120007>;
	using TechId            = GenericId<boost::uint32_t, 120008>;
	using ItemId            = GenericId<boost::uint32_t, 120009>;
	using TradeId           = GenericId<boost::uint32_t, 120010>;
	using RechargeId        = GenericId<boost::uint32_t, 120011>;
	using ShopId            = GenericId<boost::uint32_t, 120012>;

	using AccountUuid       = GenericUuid<210001>;
	using MapObjectUuid     = GenericUuid<210002>;
	using MailUuid          = GenericUuid<210003>;
}

using namespace IdTypes;

}

#endif
