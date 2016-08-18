#include "precompiled.hpp"
#include "account.hpp"
#include "mmain.hpp"
#include "mysql/legion.hpp"
#include "singletons/legion_map.hpp"
#include "msg/cs_legion.hpp"
#include "msg/sc_legion.hpp"
#include "player_session.hpp"
#include "legion.hpp"
#include "legion_member.hpp"
#include "legion_member_attribute_ids.hpp"
#include "singletons/account_map.hpp"
#include "singletons/legion_member_map.hpp"
#include "account_attribute_ids.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>

namespace EmperyCenter {

std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<LegionMember>> LegionMember::async_create(
	LegionUuid legion_uuid,  AccountUuid account_uuid,  std::uint64_t created_time)
{
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::Center_Legion_Member>(account_uuid.get(),legion_uuid.get() ,created_time);
	obj->enable_auto_saving();
	auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);
	auto account = boost::make_shared<LegionMember>(std::move(obj), std::vector<boost::shared_ptr<MySql::Center_LegionMemberAttribute>>());
	return std::make_pair(std::move(promise), std::move(account));
}

LegionMember::LegionMember(boost::shared_ptr<MySql::Center_Legion_Member> obj,
	const std::vector<boost::shared_ptr<MySql::Center_LegionMemberAttribute>> &attributes)
	: m_obj(std::move(obj))
{
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(LegionMemberAttributeId((*it)->get_legion_member_attribute_id()), *it);
	}

}
LegionMember::~LegionMember(){
}

LegionUuid LegionMember::get_legion_uuid() const {
	return LegionUuid(m_obj->unlocked_get_legion_uuid());
}

AccountUuid LegionMember::get_account_uuid() const{
	return AccountUuid(m_obj->unlocked_get_account_uuid());
}

void LegionMember::InitAttributes(boost::shared_ptr<Account> account,unsigned nTitleid)
{
	PROFILE_ME;

	// 设置属性
	boost::container::flat_map<LegionMemberAttributeId, std::string> modifiers;
	modifiers.emplace(LegionMemberAttributeIds::ID_TITLEID, boost::lexical_cast<std::string>(nTitleid));
	const auto donate = account->get_attribute(AccountAttributeIds::ID_DONATE);
	if(donate.empty() || donate == Poseidon::EMPTY_STRING)
	{
		modifiers.emplace(LegionMemberAttributeIds::ID_DONATE, "0");
	}
	else
		modifiers.emplace(LegionMemberAttributeIds::ID_DONATE, donate);
	const auto weekdonate = account->get_attribute(AccountAttributeIds::ID_WEEKDONATE);
	if(weekdonate.empty()  || weekdonate == Poseidon::EMPTY_STRING)
	{
		modifiers.emplace(LegionMemberAttributeIds::ID_WEEKDONATE, "0");
	}
	else
		modifiers.emplace(LegionMemberAttributeIds::ID_WEEKDONATE, weekdonate);

	const auto exchange_record = account->get_attribute(AccountAttributeIds::ID_LEGION_STORE_EXCHANGE_RECORD);
	if(exchange_record.empty()  || exchange_record == Poseidon::EMPTY_STRING)
	{
		modifiers.emplace(LegionMemberAttributeIds::ID_LEGION_STORE_EXCHANGE_RECORD, "");
	}
	else
		modifiers.emplace(LegionMemberAttributeIds::ID_LEGION_STORE_EXCHANGE_RECORD, exchange_record);

	modifiers.emplace(LegionMemberAttributeIds::ID_SPEAKFLAG, "0");

	set_attributes(std::move(modifiers));

}

void LegionMember::leave()
{
	// 删除属性表
	std::string strsql = "DELETE FROM Center_LegionMemberAttribute WHERE account_uuid='";
	strsql += get_account_uuid().str();
	strsql += "';";

	Poseidon::MySqlDaemon::enqueue_for_deleting("Center_LegionMemberAttribute",strsql);
}

std::uint64_t LegionMember::get_created_time() const {
	return m_obj->get_join_time();
}

const std::string &LegionMember::get_attribute(LegionMemberAttributeId account_attribute_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(account_attribute_id);
	if(it == m_attributes.end()){
		return Poseidon::EMPTY_STRING;
	}
	return it->second->unlocked_get_value();
}
void LegionMember::get_attributes(boost::container::flat_map<LegionMemberAttributeId, std::string> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->unlocked_get_value();
	}
}
void LegionMember::set_attributes(boost::container::flat_map<LegionMemberAttributeId, std::string> modifiers){
	PROFILE_ME;


	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto obj_it = m_attributes.find(it->first);
		if(obj_it == m_attributes.end()){
			auto obj = boost::make_shared<MySql::Center_LegionMemberAttribute>(m_obj->get_account_uuid(),
				it->first.get(), std::string());
			obj->async_save(true);
			m_attributes.emplace(it->first, std::move(obj));
		}
	}

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto &obj = m_attributes.at(it->first);
		obj->set_value(std::move(it->second));

		LOG_EMPERY_CENTER_DEBUG("LegionMember::set_attributes  account_uuid= ", m_obj->get_account_uuid(), " key:",it->first,"  value:",std::move(it->second));
	}

	LegionMemberMap::update(virtual_shared_from_this<LegionMember>(), false);
}

void LegionMember::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	/*
	Msg::SC_LegionInfo msg;
	msg.legion_uuid     		= 	get_legion_uuid().str();
	msg.legion_name     		= 	get_nick();
	// 要根据军团长id 获得军团长信息
	const auto leader_account = AccountMap::require(AccountUuid(get_attribute(LegionAttributeIds::ID_LEADER)));
	msg.legion_leadername    	= 	get_attribute(LegionAttributeIds::ID_LEADER);
	msg.legion_icon     		= 	get_attribute(LegionAttributeIds::ID_ICON);
	msg.legion_notice     		= 	get_attribute(LegionAttributeIds::ID_NOTICE);
	msg.legion_level     		= 	get_attribute(LegionAttributeIds::ID_LEVEL);
	msg.legion_rank     		= 	get_attribute(LegionAttributeIds::ID_RANK);
	msg.legion_money     		= 	get_attribute(LegionAttributeIds::ID_MONEY);

	// 个人信息
	msg.legion_titleid     		= 	"0";
	msg.legion_donate     		= 	"10000";
	// 军团成员数量
	msg.legion_member_count     = 	m_members.size();
	session->send(msg);
	*/
	/*
	Msg::SC_LegionInfo msg;
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
	*/
}

}
