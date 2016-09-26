#include "precompiled.hpp"
#include "league.hpp"
#include "singletons/league_map.hpp"
#include "singletons/league_member_map.hpp"
#include <poseidon/json.hpp>
#include "mmain.hpp"
#include "league_member.hpp"
#include "league_member_attribute_ids.hpp"
#include "../../empery_center/src/msg/ls_league.hpp"
#include "data/global.hpp"
#include "singletons/league_invitejoin_map.hpp"
#include "singletons/league_applyjoin_map.hpp"
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
	modifiers.emplace(LeagueAttributeIds::ID_MEMBER_MAX, Data::Global::as_string(Data::Global::SLOT_LEGAUE_CREATE_DEFAULT_MEMBERCOUNT));
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

		// 发邮件
		sendemail(EmperyCenter::ChatMessageTypeId(EmperyCenter::ChatMessageTypeIds::ID_LEVEL_LEAGUE_JOIN),legion_uuid,get_nick());
		// 广播通知
		sendNoticeMsg(League::LEAGUE_NOTICE_MSG_TYPE::LEAGUE_NOTICE_MSG_TYPE_JOIN,"",legion_uuid.str());
	}
}

void League::synchronize_with_player(const boost::shared_ptr<LeagueSession>& league_client, AccountUuid account_uuid,LegionUuid legion_uuid, std::string str_league_uuid) const
{
	PROFILE_ME;

	EmperyCenter::Msg::LS_LeagueInfo msg;
	msg.res = 0;
	msg.rewrite = 0;
	if(get_league_uuid().str() != str_league_uuid)
		msg.rewrite = 1;
	msg.account_uuid 			= 	account_uuid.str();
	msg.league_uuid     		= 	get_league_uuid().str();
	msg.league_name     		= 	get_nick();
	msg.league_icon     		= 	get_attribute(LeagueAttributeIds::ID_ICON);
	msg.league_notice     		= 	get_attribute(LeagueAttributeIds::ID_CONTENT);
	msg.league_level     		= 	boost::lexical_cast<std::uint64_t>(get_attribute(LeagueAttributeIds::ID_LEVEL));
	msg.league_max_member		=   boost::lexical_cast<std::uint64_t>(get_attribute(LeagueAttributeIds::ID_MEMBER_MAX));
	msg.leader_legion_uuid		=   get_attribute(LeagueAttributeIds::ID_LEADER);
//	msg.create_account_uuid		=   get_create_uuid().str();

	// 找到联盟成员
	std::vector<boost::shared_ptr<LeagueMember>> members;
	LeagueMemberMap::get_by_league_uuid(members, get_league_uuid());

	LOG_EMPERY_LEAGUE_INFO("get_by_league_uuid league members size*******************",members.size());

	msg.members.reserve(members.size());
	for(auto it = members.begin(); it != members.end(); ++it )
	{
		auto &elem = *msg.members.emplace(msg.members.end());
		auto info = *it;

		elem.legion_uuid = info->get_legion_uuid().str();
		elem.titleid = boost::lexical_cast<std::uint64_t>(info->get_attribute(LeagueMemberAttributeIds::ID_TITLEID));

		elem.quit_time = info->get_attribute(LeagueMemberAttributeIds::ID_QUITWAITTIME);
		elem.kick_time = info->get_attribute(LeagueMemberAttributeIds::ID_KICKWAITTIME);
		if(info->get_legion_uuid().str() == get_attribute(LeagueAttributeIds::ID_ATTORNLEADER))
			elem.attorn_time = get_attribute(LeagueAttributeIds::ID_ATTORNTIME);
		else
			elem.attorn_time = "";

	}

	league_client->send(msg);

}

void League::disband()
{
	PROFILE_ME;
	// 清除联盟属性
	for(auto it = m_attributes.begin(); it != m_attributes.end(); )
	{
		it =  m_attributes.erase(it);
	}

	// 从数据库中删除联盟属性
	std::string strsql = "DELETE FROM League_LeagueAttribute WHERE league_uuid='";
	strsql += get_league_uuid().str();
	strsql += "';";

	Poseidon::MySqlDaemon::enqueue_for_deleting("League_LeagueAttribute",strsql);

	// 联盟解散的成员的善后操作
	LeagueMemberMap::disband_league(get_league_uuid());

	// 清空请求加入的数据
	LeagueApplyJoinMap::deleteInfo_by_league_uuid(get_league_uuid());

	// 清空邀请加入的数据
	LeagueInviteJoinMap::deleteInfo_by_invited_uuid(get_league_uuid());
}

void League::set_controller(const boost::shared_ptr<LeagueSession> &controller)
{
	PROFILE_ME;

	m_leaguesession = controller;

	LeagueMap::update(virtual_shared_from_this<League>(), false);
}

void League::sendNoticeMsg(std::uint64_t msgtype,std::string nick,std::string ext1)
{
	PROFILE_ME;

	// 找到联盟成员
	try
	{
		std::vector<boost::shared_ptr<LeagueMember>> members;
		LeagueMemberMap::get_by_league_uuid(members, get_league_uuid());
		if(!members.empty())
		{
			LOG_EMPERY_LEAGUE_INFO(" sendNoticeMsg get_by_league_uuid league members size*******************",members.size());

			EmperyCenter::Msg::LS_LeagueNoticeMsg msg;
			msg.league_uuid = get_league_uuid().str();
			msg.msgtype = msgtype;
			msg.nick = nick;
			msg.ext1 = ext1;
			msg.legions.reserve(members.size());
			for(auto it = members.begin(); it != members.end(); ++it )
			{
				auto &elem = *msg.legions.emplace(msg.legions.end());
				auto info = *it;

				elem.legion_uuid = info->get_legion_uuid().str();
			}

			if(get_controller())
				get_controller()->send(msg);
		}
	}
	catch (const std::exception&)
	{

	}
	catch(...)
	{

	}

}

void League::sendemail(EmperyCenter::ChatMessageTypeId ntype,LegionUuid legion_uuid,std::string strnick,std::string str_account_uuid)
{
	PROFILE_ME;
	// 找到联盟成员
	try
	{
		LOG_EMPERY_LEAGUE_ERROR(" sendemail *******************",ntype," legion_uuid:",legion_uuid," strnick:",strnick," str_account_uuid:",str_account_uuid);

		EmperyCenter::Msg::LS_LeagueEamilMsg msg;
		msg.ntype = boost::lexical_cast<std::string>(ntype);
		msg.legion_uuid = legion_uuid.str();
		msg.ext1 = strnick;
		msg.mandator = str_account_uuid;

		if(get_controller())
			get_controller()->send(msg);

	}
	catch (const std::exception&)
	{

	}
	catch(...)
	{

	}


}

}
