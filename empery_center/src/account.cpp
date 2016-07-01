#include "precompiled.hpp"
#include "account.hpp"
#include "mmain.hpp"
#include "mysql/account.hpp"
#include "singletons/account_map.hpp"
#include "msg/sc_account.hpp"
#include "player_session.hpp"

namespace EmperyCenter {

Account::Account(AccountUuid account_uuid, PlatformId platformId, std::string login_name,
	AccountUuid referrer_uuid, unsigned promotion_level, std::uint64_t created_time, std::string nick)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_Account>(account_uuid.get(), platformId.get(), std::move(login_name),
				referrer_uuid.get(), promotion_level, created_time, std::move(nick), false, 0, 0);
			obj->async_save(true, true);
			return obj;
		}())
{
}
Account::Account(boost::shared_ptr<MySql::Center_Account> obj,
	const std::vector<boost::shared_ptr<MySql::Center_AccountAttribute>> &attributes)
	: m_obj(std::move(obj))
{
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(AccountAttributeId((*it)->get_account_attribute_id()), *it);
	}
}
Account::~Account(){
}

AccountUuid Account::get_account_uuid() const {
	return AccountUuid(m_obj->unlocked_get_account_uuid());
}
PlatformId Account::get_platform_id() const {
	return PlatformId(m_obj->get_platform_id());
}
const std::string &Account::get_login_name() const {
	return m_obj->unlocked_get_login_name();
}
AccountUuid Account::get_referrer_uuid() const {
	return AccountUuid(m_obj->unlocked_get_referrer_uuid());
}

unsigned Account::get_promotion_level() const {
	return m_obj->get_promotion_level();
}
void Account::set_promotion_level(unsigned promotion_level){
	PROFILE_ME;

	m_obj->set_promotion_level(promotion_level);
	m_obj->async_save(true, true);

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}

const std::string &Account::get_nick() const {
	return m_obj->unlocked_get_nick();
}
void Account::set_nick(std::string nick){
	PROFILE_ME;

	m_obj->set_nick(std::move(nick));
	m_obj->async_save(true, true);

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}

std::uint64_t Account::get_created_time() const {
	return m_obj->get_created_time();
}

bool Account::has_been_activated() const {
	return m_obj->get_activated();
}
void Account::set_activated(bool activated){
	PROFILE_ME;

	m_obj->set_activated(activated);
	m_obj->async_save(true, true);

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}

std::uint64_t Account::get_banned_until() const {
	return m_obj->get_banned_until();
}
void Account::set_banned_until(std::uint64_t banned_until){
	m_obj->set_banned_until(banned_until);
	m_obj->async_save(true, true);

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}

std::uint64_t Account::get_quieted_until() const {
	return m_obj->get_quieted_until();
}
void Account::set_quieted_until(std::uint64_t quieted_until){
	m_obj->set_quieted_until(quieted_until);
	m_obj->async_save(true, true);

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}

const std::string &Account::get_attribute(AccountAttributeId account_attribute_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(account_attribute_id);
	if(it == m_attributes.end()){
		return Poseidon::EMPTY_STRING;
	}
	return it->second->unlocked_get_value();
}
void Account::get_attributes(boost::container::flat_map<AccountAttributeId, std::string> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->unlocked_get_value();
	}
}
void Account::set_attributes(boost::container::flat_map<AccountAttributeId, std::string> modifiers){
	PROFILE_ME;

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto obj_it = m_attributes.find(it->first);
		if(obj_it == m_attributes.end()){
			auto obj = boost::make_shared<MySql::Center_AccountAttribute>(m_obj->get_account_uuid(),
				it->first.get(), std::string());
			obj->async_save(true);
			m_attributes.emplace(it->first, std::move(obj));
		}
	}
	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto &obj = m_attributes.at(it->first);
		obj->set_value(std::move(it->second));
	}

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}

void Account::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	Msg::SC_AccountAttributes msg;
	msg.account_uuid    = get_account_uuid().str();
	msg.nick            = get_nick();
	msg.attributes.reserve(m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		auto &attribute = *msg.attributes.emplace(msg.attributes.end());
		attribute.account_attribute_id = it->first.get();
		attribute.value                = it->second->unlocked_get_value();
	}
	msg.promotion_level = get_promotion_level();
	msg.activated       = has_been_activated();
	session->send(msg);
}

}
