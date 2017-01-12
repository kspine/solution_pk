#include "precompiled.hpp"
#include "account.hpp"
#include "mmain.hpp"
#include "mysql/account.hpp"
#include "singletons/account_map.hpp"
#include "msg/sc_account.hpp"
#include "player_session.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/vip.hpp"
#include "account_attribute_ids.hpp"
#include "item_box.hpp"
#include "singletons/item_box_map.hpp"
#include "item_ids.hpp"
#include "transaction_element.hpp"
#include "reason_ids.hpp"

namespace EmperyCenter {

std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<Account>> Account::async_create(
	AccountUuid account_uuid, PlatformId platformId, std::string login_name,
	AccountUuid referrer_uuid, unsigned promotion_level, std::uint64_t created_time, std::string nick)
{
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::Center_Account>(account_uuid.get(), platformId.get(), std::move(login_name),
		referrer_uuid.get(), promotion_level, created_time, std::move(nick), false, 0, 0);
	obj->enable_auto_saving();
	auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);
	auto account = boost::make_shared<Account>(std::move(obj), std::vector<boost::shared_ptr<MySql::Center_AccountAttribute>>());
	return std::make_pair(std::move(promise), std::move(account));
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

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}

const std::string &Account::get_nick() const {
	return m_obj->unlocked_get_nick();
}
void Account::set_nick(std::string nick){
	PROFILE_ME;

	m_obj->set_nick(std::move(nick));

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

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}

std::uint64_t Account::get_banned_until() const {
	return m_obj->get_banned_until();
}
void Account::set_banned_until(std::uint64_t banned_until){
	m_obj->set_banned_until(banned_until);

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}

std::uint64_t Account::get_quieted_until() const {
	return m_obj->get_quieted_until();
}
void Account::set_quieted_until(std::uint64_t quieted_until){
	m_obj->set_quieted_until(quieted_until);

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

void Account::add_vip_exp(std::uint64_t exp){
	PROFILE_ME;

	if(exp == 0){
		LOG_EMPERY_CENTER_WARNING("add exp zero, account_uuid = ",get_account_uuid());
		return;
	}
	const auto vip_exp = cast_attribute<std::uint64_t>(AccountAttributeIds::ID_VIP_EXP);
	const auto vip_level = cast_attribute<unsigned>(AccountAttributeIds::ID_VIP_LEVEL);
	const auto vip_data = Data::Vip::get(vip_level+1);
	if(!vip_data){
		LOG_EMPERY_CENTER_WARNING("no vip data,vip_level ",vip_level + 1);
		return;
	}
	LOG_EMPERY_CENTER_DEBUG("before add vip exp,account_uuid = ",get_account_uuid()," old_vip_level = ",vip_level, " old_vip_exp = ",vip_exp," delta = ",exp);
	auto total_exp = vip_exp + exp;
	auto next_level_exp = vip_data->vip_exp;
	auto new_vip_level = vip_level;
	while((total_exp > next_level_exp) && (next_level_exp > 0)){
		total_exp -= next_level_exp;
		new_vip_level += 1;
		const auto vip_data = Data::Vip::get(new_vip_level+1);
		if(vip_data){
			next_level_exp = vip_data->vip_exp;
		}else{
			LOG_EMPERY_CENTER_WARNING("no vip data,vip_level = ",new_vip_level + 1);
			next_level_exp = 0;
		}
	}
	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers.reserve(2);
	modifiers[AccountAttributeIds::ID_VIP_LEVEL] = boost::lexical_cast<std::string>(new_vip_level);
	modifiers[AccountAttributeIds::ID_VIP_EXP]   = boost::lexical_cast<std::string>(total_exp);;
	set_attributes(std::move(modifiers));
	LOG_EMPERY_CENTER_DEBUG("end add vip exp,account_uuid = ",get_account_uuid()," new_vip_level = ",new_vip_level, " new_vip_exp = ",total_exp);
	if(vip_level != new_vip_level){
		on_vip_level_changed(vip_level,new_vip_level);
	}
}

void Account::on_vip_level_changed(unsigned old_level,unsigned new_level){
	PROFILE_ME;
	if(old_level >= new_level){
		LOG_EMPERY_CENTER_WARNING("on vip level changed , old_vip_level = ",old_level, " new_vip_level = ",new_level);
		return;
	}

	//增加副本进入次数
	auto item_box = ItemBoxMap::require(get_account_uuid());
	std::vector<ItemTransactionElement> transaction;
	transaction.reserve(1);
	std::uint64_t delta = 0;
	for(unsigned i = old_level + 1;i <= new_level;++i){
		const auto vip_data = Data::Vip::get(i);
		if(!vip_data){
			LOG_EMPERY_CENTER_WARNING("no vip data,vip level = ",i);
		}
		delta += vip_data->dungeon_count;
	}
	transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemIds::ID_DUNGEON_ENTER_COUNT, delta,
			ReasonIds::ID_DUNGEON_COUNT_VIP_CHANGED, old_level, new_level, 0);
	item_box->commit_transaction(transaction, false);
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
	msg.online          = (PlayerSessionMap::get(get_account_uuid()) != NULL);
	session->send(msg);
}

}
