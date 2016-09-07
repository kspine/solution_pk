#include "precompiled.hpp"
#include "league.hpp"
#include "singletons/league_map.hpp"
#include "singletons/league_member_map.hpp"
#include <poseidon/json.hpp>
#include "mmain.hpp"
#include "league_member.hpp"
#include "league_member_attribute_ids.hpp"
#include "../../empery_center/src/msg/ls_league.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>


namespace EmperyLeague {
//namespace Msg = ::EmperyLeague::Msg;

std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<League>> League::async_create(
	LeagueUuid league_uuid,  LegionUuid legion_uuid, std::string league_name, AccountUuid account_uuid, std::uint64_t created_time)
{
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::League_Info>(league_uuid.get(), std::move(league_name),legion_uuid.get(),account_uuid.get(),created_time);
	obj->enable_auto_saving();
	auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);
	auto account = boost::make_shared<League>(std::move(obj), std::vector<boost::shared_ptr<MySql::League_LeagueAttribute>>());
	return std::make_pair(std::move(promise), std::move(account));
}

League::League(boost::shared_ptr<MySql::League_Info> obj,
	const std::vector<boost::shared_ptr<MySql::League_LeagueAttribute>> &attributes)
	: m_obj(std::move(obj))
{
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(LeagueAttributeId((*it)->get_league_attribute_id()), *it);
	}

//	LOG_EMPERY_CENTER_INFO("legion members size==============================================",m_members.size());
//	sleep(5000);
}



League::~League(){
}

void League::pump_status(){
	PROFILE_ME;

	//
}

void League::set_founder_uuid(AccountUuid founder_uuid){
	PROFILE_ME;

	LeagueMap::update(virtual_shared_from_this<League>(), false);
}

const std::string &League::get_nick() const {
	return m_obj->unlocked_get_name();
}

const std::string &League::get_attribute(LeagueAttributeId account_attribute_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(account_attribute_id);
	if(it == m_attributes.end()){
		return Poseidon::EMPTY_STRING;
	}
	return it->second->unlocked_get_value();
}
void League::get_attributes(boost::container::flat_map<LeagueAttributeId, std::string> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->unlocked_get_value();
	}
}
void League::set_attributes(boost::container::flat_map<LeagueAttributeId, std::string> modifiers){
	PROFILE_ME;


	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto obj_it = m_attributes.find(it->first);
		if(obj_it == m_attributes.end()){
			auto obj = boost::make_shared<MySql::League_LeagueAttribute>(m_obj->get_league_uuid(),
				it->first.get(), std::string());
			obj->async_save(true);
			m_attributes.emplace(it->first, std::move(obj));
		}
	}

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto &obj = m_attributes.at(it->first);
		obj->set_value(std::move(it->second));

		LOG_EMPERY_LEAGUE_DEBUG("League::set_attributes  account_uuid= ", obj->get_league_uuid(), " key:",it->first,"  value:",std::move(it->second));
	}

	LeagueMap::update(virtual_shared_from_this<League>(), false);
}

void League::InitAttributes(LegionUuid legion_uuid,std::string content, std::string language, std::string icon,unsigned bautojoin)
{
	PROFILE_ME;

	// 设置属性
	boost::container::flat_map<LeagueAttributeId, std::string> modifiers;
	modifiers.emplace(LeagueAttributeIds::ID_LEADER, legion_uuid.str());
	modifiers.emplace(LeagueAttributeIds::ID_LEVEL, "1");
	modifiers.emplace(LeagueAttributeIds::ID_CONTENT, std::move(content));
//	modifiers.emplace(LegionAttributeIds::ID_NOTICE, "");
	modifiers.emplace(LeagueAttributeIds::ID_ICON, std::move(icon));
	modifiers.emplace(LeagueAttributeIds::ID_LANAGE, std::move(language));
	if(bautojoin)
	{
		modifiers.emplace(LeagueAttributeIds::ID_AUTOJOIN, "1");
	}
	else
	{
		modifiers.emplace(LeagueAttributeIds::ID_AUTOJOIN,"0");
	}
//	modifiers.emplace(LeagueAttributeIds::ID_MONEY, "0");
//	modifiers.emplace(LeagueAttributeIds::ID_RANK, "0");
	set_attributes(std::move(modifiers));
}

void League::AddMember(LegionUuid legion_uuid,AccountUuid account_uuid,unsigned level,std::uint64_t join_time)
{
	PROFILE_ME;

	// 先判断玩家是否已经加入军团
	const auto member = LeagueMemberMap::get_by_legion_uuid(legion_uuid);
	if(member)
	{
		LOG_EMPERY_LEAGUE_ERROR("AddMember Find same  legion_uuid:", legion_uuid);
	}
	else
	{
		auto pair = LeagueMember::async_create( get_league_uuid(),legion_uuid,join_time);
		Poseidon::JobDispatcher::yield(pair.first, true);

		auto member = std::move(pair.second);

		// 设置成员属性
		member->InitAttributes(legion_uuid,level);

		LOG_EMPERY_LEAGUE_DEBUG("AddMember legion_uuid:", legion_uuid,"  join_time:",join_time," rank:",level);

		LeagueMemberMap::insert(member, std::string());
	}
}

void League::synchronize_with_player(const boost::shared_ptr<LeagueSession>& league_client, AccountUuid account_uuid,LegionUuid legion_uuid) const
{
	PROFILE_ME;

	EmperyCenter::Msg::LS_LeagueInfo msg;
	msg.account_uuid 			= 	account_uuid.str();
	msg.league_uuid     		= 	get_league_uuid().str();
	msg.league_name     		= 	get_nick();
	msg.league_icon     		= 	get_attribute(LeagueAttributeIds::ID_ICON);
	msg.league_notice     		= 	get_attribute(LeagueAttributeIds::ID_CONTENT);
	msg.league_level     		= 	boost::lexical_cast<std::uint64_t>(get_attribute(LeagueAttributeIds::ID_LEVEL));
	msg.leader_legion_uuid		=   get_attribute(LeagueAttributeIds::ID_LEADER);
	msg.create_account_uuid		=   get_create_uuid().str();

	// 根据account_uuid查找是否有军团
	std::vector<boost::shared_ptr<LeagueMember>> members;
	LeagueMemberMap::get_by_league_uuid(members, get_league_uuid());

	LOG_EMPERY_LEAGUE_INFO("get_by_league_uuid league members size*******************",members.size());

	msg.members.reserve(members.size());
	for(auto it = members.begin(); it != members.end(); ++it )
	{
		auto &elem = *msg.members.emplace(msg.members.end());
		auto info = *it;

		elem.legion_uuid = info->get_legion_uuid().str();
		elem.speakflag = boost::lexical_cast<std::uint64_t>(info->get_attribute(LeagueMemberAttributeIds::ID_SPEAKFLAG));
		elem.titleid = boost::lexical_cast<std::uint64_t>(info->get_attribute(LeagueMemberAttributeIds::ID_TITLEID));

	}

	league_client->send(msg);

}

}
