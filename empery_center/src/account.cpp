#include "precompiled.hpp"
#include "account.hpp"
#include "mysql/account.hpp"
#include "singletons/account_map.hpp"
#include "msg/sc_account.hpp"
#include "player_session.hpp"
#include "singletons/player_session_map.hpp"
#include "castle.hpp"
#include "singletons/world_map.hpp"
#include "map_object_type_ids.hpp"

namespace EmperyCenter {

Account::Account(AccountUuid account_uuid, PlatformId platformId, std::string login_name,
	AccountUuid referrer_uuid, unsigned promotion_level, std::uint64_t created_time, std::string nick)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_Account>(account_uuid.get(), platformId.get(), std::move(login_name),
				referrer_uuid.get(), promotion_level, created_time, std::move(nick), false, std::string(), 0, 0);
			obj->async_save(true);
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
void Account::set_referrer_uuid(AccountUuid account_uuid){
	PROFILE_ME;

	m_obj->set_referrer_uuid(account_uuid.get());

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}

unsigned Account::get_promotion_level() const {
	return m_obj->get_promotion_level();
}
void Account::set_promotion_level(unsigned promotion_level){
	PROFILE_ME;

	m_obj->set_promotion_level(promotion_level);

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}

const std::string &Account::get_nick() const {
	return m_obj->unlocked_get_nick();
}
void Account::set_nick(std::string nick){
	PROFILE_ME;

	m_obj->set_nick(std::move(nick));

	AccountMap::update(virtual_shared_from_this<Account>(), false);

	const auto account_uuid = get_account_uuid();
	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			Msg::SC_AccountAttributes msg;
			msg.account_uuid    = account_uuid.str();
			msg.nick            = m_obj->unlocked_get_nick();
			msg.promotion_level = get_promotion_level();
			msg.activated       = has_been_activated();
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

std::uint64_t Account::get_created_time() const {
	return m_obj->get_created_time();
}

bool Account::has_been_activated() const {
	return m_obj->get_activated();
}
void Account::activate(){
	PROFILE_ME;

	if(has_been_activated()){
		return;
	}

	boost::shared_ptr<Castle> castle;
	std::vector<boost::shared_ptr<MapObject>> map_objects;
	WorldMap::get_map_objects_by_owner(map_objects, get_account_uuid());
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = *it;
		const auto test_castle = boost::dynamic_pointer_cast<Castle>(map_object);
		if(test_castle){
			castle = std::move(test_castle);
			break;
		}
	}
	if(!castle){
		castle = WorldMap::create_init_castle(
			[&](Coord coord){
				return boost::make_shared<Castle>(MapObjectUuid(Poseidon::Uuid::random()),
					get_account_uuid(), MapObjectUuid(), get_nick(), coord);
			},
			Coord(-1,0) // TODO init map
		);
		if(!castle){
			LOG_EMPERY_CENTER_WARNING("Failed to create initial castle! account_uuid = ", get_account_uuid());
			DEBUG_THROW(Exception, sslit("Failed to create initial castle"));
		}
	}

	m_obj->set_activated(true);
}

const std::string &Account::get_login_token() const {
	return m_obj->unlocked_get_login_token();
}
std::uint64_t Account::get_login_token_expiry_time() const {
	return m_obj->get_login_token_expiry_time();
}
void Account::set_login_token(std::string login_token, std::uint64_t login_token_expiry_time){
	m_obj->set_login_token(std::move(login_token));
	m_obj->set_login_token_expiry_time(login_token_expiry_time);
}

std::uint64_t Account::get_banned_until() const {
	return m_obj->get_banned_until();
}
void Account::set_banned_until(std::uint64_t banned_until){
	m_obj->set_banned_until(banned_until);
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
			obj->enable_auto_saving(); // obj->async_save(true);
			m_attributes.emplace(it->first, std::move(obj));
		}
	}
	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto &obj = m_attributes.at(it->first);
		obj->set_value(std::move(it->second));
	}

	const auto account_uuid = get_account_uuid();
	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			Msg::SC_AccountAttributes msg;
			msg.account_uuid = account_uuid.str();
			msg.nick            = m_obj->unlocked_get_nick();
			msg.attributes.reserve(modifiers.size());
			for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
				const auto &obj = m_attributes.at(it->first);

				auto &attribute = *msg.attributes.emplace(msg.attributes.end());
				attribute.account_attribute_id = obj->get_account_attribute_id();
				attribute.value                = obj->unlocked_get_value();
			}
			msg.promotion_level = get_promotion_level();
			msg.activated       = has_been_activated();
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

}
