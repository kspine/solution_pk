#include "precompiled.hpp"
#include "mmain.hpp"
#include "mysql/league.hpp"
#include "league_member.hpp"
#include "league_member_attribute_ids.hpp"
#include "singletons/league_member_map.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>

namespace EmperyLeague {

std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<LeagueMember>> LeagueMember::async_create(
	LeagueUuid league_uuid,  LegionUuid legion_uuid,  std::uint64_t created_time)
{
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::League_Member>(legion_uuid.get() ,league_uuid.get(),created_time);
	obj->enable_auto_saving();
	auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);
	auto account = boost::make_shared<LeagueMember>(std::move(obj), std::vector<boost::shared_ptr<MySql::League_MemberAttribute>>());
	return std::make_pair(std::move(promise), std::move(account));
}

LeagueMember::LeagueMember(boost::shared_ptr<MySql::League_Member> obj,
	const std::vector<boost::shared_ptr<MySql::League_MemberAttribute>> &attributes)
	: m_obj(std::move(obj))
{
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(LeagueMemberAttributeId((*it)->get_league_member_attribute_id()), *it);
	}

}
LeagueMember::~LeagueMember(){
}

LeagueUuid LeagueMember::get_league_uuid() const {
	return LeagueUuid(m_obj->unlocked_get_league_uuid());
}

LegionUuid LeagueMember::get_legion_uuid() const{
	return LegionUuid(m_obj->unlocked_get_legion_uuid());
}

void LeagueMember::InitAttributes(LegionUuid legion_uuid,unsigned nTitleid)
{
	PROFILE_ME;

	// 设置属性
	boost::container::flat_map<LeagueMemberAttributeId, std::string> modifiers;
	modifiers.emplace(LeagueMemberAttributeIds::ID_TITLEID, boost::lexical_cast<std::string>(nTitleid));
	modifiers.emplace(LeagueMemberAttributeIds::ID_SPEAKFLAG, "0");
	set_attributes(std::move(modifiers));

}

void LeagueMember::leave()
{
	// 删除属性表

	std::string strsql = "DELETE FROM League_MemberAttribute WHERE legion_uuid='";
	strsql += get_legion_uuid().str();
	strsql += "';";

	Poseidon::MySqlDaemon::enqueue_for_deleting("League_MemberAttribute",strsql);

}

std::uint64_t LeagueMember::get_created_time() const {
	return m_obj->get_join_time();
}

const std::string &LeagueMember::get_attribute(LeagueMemberAttributeId account_attribute_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(account_attribute_id);
	if(it == m_attributes.end()){
		return Poseidon::EMPTY_STRING;
	}
	return it->second->unlocked_get_value();
}
void LeagueMember::get_attributes(boost::container::flat_map<LeagueMemberAttributeId, std::string> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->unlocked_get_value();
	}
}
void LeagueMember::set_attributes(boost::container::flat_map<LeagueMemberAttributeId, std::string> modifiers){
	PROFILE_ME;


	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto obj_it = m_attributes.find(it->first);
		if(obj_it == m_attributes.end()){
			auto obj = boost::make_shared<MySql::League_MemberAttribute>(m_obj->get_legion_uuid(),
				it->first.get(), std::string());
			obj->async_save(true);
			m_attributes.emplace(it->first, std::move(obj));
		}
	}

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto &obj = m_attributes.at(it->first);
		obj->set_value(std::move(it->second));

		LOG_EMPERY_LEAGUE_DEBUG("LeagueMember::set_attributes  legion_uuid= ", m_obj->get_legion_uuid(), " key:",it->first,"  value:",std::move(it->second));
	}

	LeagueMemberMap::update(virtual_shared_from_this<LeagueMember>(), false);
}

void LeagueMember::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	/*
	Msg::SC_LegionInfo msg;
	msg.legion_uuid     		= 	get_legion_uuid().str();
	msg.legion_name     		= 	get_nick();
	// 要根据军团长id 获得军团长信息
	const auto leader_account = AccountMap::require(AccountUuid(get_attribute(LegionAttributeIds::ID_LEADER)));
	msg.legion_leadername    	= 	get_attribute(LegionAttributeIds::ID_LEADER);
	msg.legion_icon     		= 	get_attribute(LegionAttributeIds::ID_ICON);
	msg.legion_notice     		= 	get_attribute(LegionAttributeIds::ID_CONTENT);
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
