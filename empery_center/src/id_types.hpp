#ifndef EMPERY_CENTER_ID_TYPES_HPP_
#define EMPERY_CENTER_ID_TYPES_HPP_

#include <iosfwd>
#include <iomanip>
#include <boost/cstdint.hpp>
#include <poseidon/cxx_ver.hpp>
#include <poseidon/uuid.hpp>
#include <array>

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
		return *this == GenericId();
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
	explicit GenericUuid(const std::string &s)
		: m_uuid(s.empty() ? Poseidon::Uuid() : Poseidon::Uuid(s))
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
		if(m_uuid == Poseidon::Uuid()){
			return { };
		}
		return m_uuid.to_string();
	}

public:
	constexpr bool operator==(GenericUuid rhs) const {
		return get() == rhs.get();
	}
	constexpr bool operator!=(GenericUuid rhs) const {
		return get() != rhs.get();
	}
	constexpr bool operator<(GenericUuid rhs) const {
		return get() < rhs.get();
	}
	constexpr bool operator>(GenericUuid rhs) const {
		return get() > rhs.get();
	}
	constexpr bool operator<=(GenericUuid rhs) const {
		return get() <= rhs.get();
	}
	constexpr bool operator>=(GenericUuid rhs) const {
		return get() >= rhs.get();
	}

	constexpr explicit operator bool() const noexcept {
		return *this == GenericUuid();
	}
};

template<int MAGIC_T>
std::ostream &operator<<(std::ostream &os, const GenericUuid<MAGIC_T> &uuid){
	char str[37];
	uuid.get().to_string(reinterpret_cast<char (&)[36]>(str));
	str[36] = 0;
	os <<str;
	return os;
}
template<int MAGIC_T>
std::istream &operator>>(std::istream &is, GenericUuid<MAGIC_T> &uuid){
	char str[37];
	if(is >>std::setw(sizeof(str)) >>str){
		uuid.from_string(reinterpret_cast<const char (&)[36]>(str));
	}
	return is;
}

template<std::size_t CHARS_T, int>
class GenericName {
private:
	std::array<char, CHARS_T> m_name;

public:
	constexpr GenericName()
		: m_name()
	{
	}
	explicit constexpr GenericName(const std::array<char, CHARS_T> &name)
		: m_name(name)
	{
	}
	explicit GenericName(const std::string &str)
		: m_name()
	{
		const auto len = str.copy(m_name.data(), m_name.size());
		if(len < str.size()){
			throw std::logic_error("String for a GenericName is too long");
		}
	}

public:
	constexpr const std::array<char, CHARS_T> &get() const {
		return m_name;
	}
	void set(const std::array<char, CHARS_T> &name){
		m_name = name;
	}

	std::string str() const {
		if(m_name[0] == 0){
			return { };
		}
		std::string s(m_name.data(), m_name.size());
		s.resize(std::strlen(s.c_str()));
		return std::move(s);
	}

public:
	constexpr bool operator==(GenericName rhs) const {
		return get() == rhs.get();
	}
	constexpr bool operator!=(GenericName rhs) const {
		return get() != rhs.get();
	}
	constexpr bool operator<(GenericName rhs) const {
		return get() < rhs.get();
	}
	constexpr bool operator>(GenericName rhs) const {
		return get() > rhs.get();
	}
	constexpr bool operator<=(GenericName rhs) const {
		return get() <= rhs.get();
	}
	constexpr bool operator>=(GenericName rhs) const {
		return get() >= rhs.get();
	}

	constexpr explicit operator bool() const noexcept {
		return *this == GenericName();
	}
};

template<std::size_t CHARS_T, int MAGIC_T>
std::ostream &operator<<(std::ostream &os, const GenericName<CHARS_T, MAGIC_T> &name){
	char str[CHARS_T + 1];
	reinterpret_cast<std::array<char, CHARS_T> &>(str) = name.get();
	str[CHARS_T] = 0;
	os <<str;
	return os;
}
template<std::size_t CHARS_T, int MAGIC_T>
std::istream &operator>>(std::istream &is, GenericName<CHARS_T, MAGIC_T> &name){
	char str[CHARS_T + 1];
	if(is >>std::setw(sizeof(str)) >>str){
		name.set(reinterpret_cast<const std::array<char, CHARS_T> &>(str));
	}
	return is;
}

namespace IdTypes {
	using PlatformId          = GenericId<boost::uint32_t, 110001>;
	using LanguageId          = GenericId<boost::uint32_t, 110002>;
	using AccountAttributeId  = GenericId<boost::uint32_t, 110003>;
	using ReasonId            = GenericId<boost::uint32_t, 110004>;
	using MailTypeId          = GenericId<boost::uint32_t, 110005>;
	using ChatChannelId       = GenericId<boost::uint32_t, 110006>;
	using ChatMessageTypeId   = GenericId<boost::uint32_t, 110007>;
	using ChatMessageSlotId   = GenericId<boost::uint32_t, 110008>;

	using TerrainId           = GenericId<boost::uint32_t, 120001>;
	using OverlayId           = GenericId<boost::uint32_t, 120002>;
	using ResourceId          = GenericId<boost::uint32_t, 120003>;
	using MapObjectTypeId     = GenericId<boost::uint32_t, 120004>;
	using AttributeId         = GenericId<boost::uint32_t, 120005>;
	using BuildingBaseId      = GenericId<boost::uint32_t, 120006>;
	using BuildingId          = GenericId<boost::uint32_t, 120007>;
	using TechId              = GenericId<boost::uint32_t, 120008>;
	using ItemId              = GenericId<boost::uint32_t, 120009>;
	using TradeId             = GenericId<boost::uint32_t, 120010>;
	using RechargeId          = GenericId<boost::uint32_t, 120011>;
	using ShopId              = GenericId<boost::uint32_t, 120012>;

	using AccountUuid         = GenericUuid<210001>;
	using MapObjectUuid       = GenericUuid<210002>;
	using MailUuid            = GenericUuid<210003>;
	using ChatMessageUuid     = GenericUuid<210004>;
	using AnnouncementUuid    = GenericUuid<210005>;

	using OverlayGroupName    = GenericName<32, 310001>;
}

using namespace IdTypes;

}

#endif
