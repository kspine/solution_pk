#include "../precompiled.hpp"
#include "league_invitejoin_map.hpp"
#include "../mmain.hpp"
#include "../../../empery_center/src/msg/ls_league.hpp"
#include "../string_utilities.hpp"
#include "../mysql/league.hpp"
#include "league_map.hpp"
#include "../league_session.hpp"
#include "../league_attribute_ids.hpp"
#include "../league.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <tuple>


namespace EmperyLeague {

namespace {
	struct LegionElement {
		boost::shared_ptr<MySql::League_LeagueInviteJoin> account;

		LegionUuid legion_uuid;
		LeagueUuid league_uuid;

		explicit LegionElement(boost::shared_ptr<MySql::League_LeagueInviteJoin> account_)
			: account(std::move(account_))
			, legion_uuid(account->get_legion_uuid())
			, league_uuid(account->get_league_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(LegionContainer, LegionElement,
		MULTI_MEMBER_INDEX(legion_uuid)
		MULTI_MEMBER_INDEX(league_uuid)
	)

	boost::shared_ptr<LegionContainer> g_invitejoin_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// Legion
		struct TempLegionElement {
			boost::shared_ptr<MySql::League_LeagueInviteJoin> obj;
		};
		std::map<LegionUuid, TempLegionElement> temp_account_map;

		LOG_EMPERY_LEAGUE_INFO("Loading League_LeagueInviteJoin...");
		conn->execute_sql("SELECT * FROM `League_LeagueInviteJoin`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::League_LeagueInviteJoin>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			const auto legion_uuid = LegionUuid(obj->get_legion_uuid());
			temp_account_map[legion_uuid].obj = std::move(obj);
		}
		LOG_EMPERY_LEAGUE_INFO("Loaded ", temp_account_map.size(), " Legion(s).");

		const auto legion_map = boost::make_shared<LegionContainer>();
		for(auto it = temp_account_map.begin(); it != temp_account_map.end(); ++it){
			legion_map->insert(LegionElement(std::move(it->second.obj)));
		}
		g_invitejoin_map = legion_map;
		handles.push(legion_map);

	}

}


void LeagueInviteJoinMap::insert(const boost::shared_ptr<MySql::League_LeagueInviteJoin> &legion){
	PROFILE_ME;

	const auto &legion_map = g_invitejoin_map;
	if(!legion_map){
		LOG_EMPERY_LEAGUE_WARNING("LeagueInviteJoinMap  not loaded.");
		DEBUG_THROW(Exception, sslit("LeagueInviteJoinMap not loaded"));
	}


	const auto legion_uuid = legion->get_legion_uuid();

	LOG_EMPERY_LEAGUE_DEBUG("Inserting legion: legion_uuid = ", legion_uuid);

	if(!legion_map->insert(LegionElement(legion)).second){
		LOG_EMPERY_LEAGUE_WARNING("Legion already exists: legion_uuid = ", legion_uuid);
		DEBUG_THROW(Exception, sslit("Legion already exists"));
	}
}


boost::shared_ptr<MySql::League_LeagueInviteJoin> LeagueInviteJoinMap::find(LeagueUuid league_uuid,LegionUuid legion_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("g_invitejoin_map not loaded.");
		return { };
	}

	const auto range = account_map->equal_range<0>(legion_uuid);
	for(auto it = range.first; it != range.second; ++it){
		if(LeagueUuid(it->account->unlocked_get_league_uuid()) == league_uuid && LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid )
		{
			LOG_EMPERY_LEAGUE_WARNING("g_invitejoin_map  找到了.");
			return it->account;
		}
	}


	return { };
}

void LeagueInviteJoinMap::get_by_invited_uuid(std::vector<boost::shared_ptr<MySql::League_LeagueInviteJoin>> &ret, LeagueUuid league_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("g_invitejoin_map not loaded.");
		return;
	}

	const auto range = account_map->equal_range<1>(league_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->account);
	}
}

boost::shared_ptr<MySql::League_LeagueInviteJoin> LeagueInviteJoinMap::find_inviteinfo_by_user(LeagueUuid league_uuid,LegionUuid legion_uuid)
{
	PROFILE_ME;

	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("league map not loaded.");
		return { };
	}

	const auto range = account_map->equal_range<1>(league_uuid);
	for(auto it = range.first; it != range.second; ++it){
		if(LeagueUuid(it->account->unlocked_get_league_uuid()) == league_uuid && LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid )
		{
			LOG_EMPERY_LEAGUE_WARNING("g_invitejoin_map  找到了.");
			return it->account;
		}
	}

	return { };

}

std::uint64_t LeagueInviteJoinMap::get_apply_count(LegionUuid legion_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("league map not loaded.");
		return { };
	}

	const auto range = account_map->equal_range<0>(legion_uuid);

	return static_cast<std::uint64_t>(std::distance(range.first, range.second));
}

void LeagueInviteJoinMap::synchronize_with_player(const boost::shared_ptr<LeagueSession>& league_client,LegionUuid legion_uuid,AccountUuid account_uuid)
{
	PROFILE_ME;

	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("league map not loaded.");
		EmperyCenter::Msg::LS_InvieJoinList msg;
		msg.account_uuid = account_uuid.str();
		msg.legion_uuid = legion_uuid.str();
		msg.invites.reserve(0);
		league_client->send(msg);
		return ;
	}

	EmperyCenter::Msg::LS_InvieJoinList msg;
	msg.account_uuid = account_uuid.str();
	msg.legion_uuid = legion_uuid.str();
	const auto range = account_map->equal_range<0>(legion_uuid);
	msg.invites.reserve(static_cast<std::uint64_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it)
	{
		const auto &info = it->account;

		const auto& league = LeagueMap::get(LeagueUuid(info->unlocked_get_league_uuid()));
		if(league)
		{
			auto &elem = *msg.invites.emplace(msg.invites.end());
			elem.league_uuid = league->get_league_uuid().str();
			elem.league_name = league->get_nick();
            elem.league_icon = league->get_attribute(LeagueAttributeIds::ID_ICON);
			elem.league_leader_uuid = league->get_attribute(LeagueAttributeIds::ID_LEADER);
		}

	}

	LOG_EMPERY_LEAGUE_WARNING("league invatees count:",msg.invites.size());

	league_client->send(msg);

}

void LeagueInviteJoinMap::deleteInfo(LegionUuid legion_uuid,LeagueUuid league_uuid,bool bAll /*= false*/)
{
	PROFILE_ME;
	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("legion apply join map not loaded.");
		return;
	}

	std::string strsql = "";
	if(bAll)
	{
		// 全部删除该军团的数据,先删除内存中的,因为访问的是内存中的数据

		const auto range = account_map->equal_range<1>(league_uuid);
		for(auto it = range.first; it != range.second;){
			it = account_map->erase<1>(it);
		}


		// 再删除数据库中的
		strsql = "DELETE FROM League_LeagueInviteJoin WHERE league_uuid='";
		strsql += league_uuid.str();
		strsql += "';";

	}
	else
	{
		const auto range = account_map->equal_range<1>(league_uuid);
		for(auto it = range.first; it != range.second; ++it)
		{
			if(LeagueUuid(it->account->unlocked_get_league_uuid()) == league_uuid && LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid )
			{
				account_map->erase<1>(it);
				strsql = "DELETE FROM League_LeagueInviteJoin WHERE league_uuid='";
				strsql += league_uuid.str();
				strsql += "' AND legion_uuid='";
				strsql += legion_uuid.str();
				strsql += "';";

				break;

			}
		}
	}

	LOG_EMPERY_LEAGUE_DEBUG("Reclaiming legion apply join map strsql = ", strsql);

	Poseidon::MySqlDaemon::enqueue_for_deleting("League_LeagueInviteJoin",strsql);
}

void LeagueInviteJoinMap::deleteInfo_by_invited_uuid(LeagueUuid league_uuid)
{
	PROFILE_ME;
	// 先从内存中删除
	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("legion apply join map not loaded.");
		return;
	}

	const auto range = account_map->equal_range<1>(league_uuid);
		for(auto it = range.first; it != range.second;){
			it = account_map->erase<1>(it);
		}

	// 再从数据库中删除
	std::string strsql = "DELETE FROM League_LeagueInviteJoin WHERE league_uuid='";
	strsql += league_uuid.str();
	strsql += "';";

	Poseidon::MySqlDaemon::enqueue_for_deleting("League_LeagueInviteJoin",strsql);
}

void LeagueInviteJoinMap::deleteInfo_by_legion_uuid(LegionUuid legion_uuid)
{
	PROFILE_ME;
	// 先从内存中删除
	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("legion apply join map not loaded.");
		return;
	}

	const auto range = account_map->equal_range<0>(legion_uuid);
		for(auto it = range.first; it != range.second;){
			it = account_map->erase<0>(it);
		}

	// 再从数据库中删除
	std::string strsql = "DELETE FROM League_LeagueInviteJoin WHERE legion_uuid='";
	strsql += legion_uuid.str();
	strsql += "';";

	Poseidon::MySqlDaemon::enqueue_for_deleting("League_LeagueInviteJoin",strsql);
}

void LeagueInviteJoinMap::deleteInfo_by_invitedres_uuid(LeagueUuid league_uuid,LegionUuid legion_uuid,bool bAll/* = false*/)
{
	PROFILE_ME;

	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("legion apply join map not loaded.");
		return;
	}

	const auto range = account_map->equal_range<1>(league_uuid);
	std::string strsql = "";
	if(bAll)
	{
		// 先从内存中删除
		for(auto it = range.first; it != range.second;){
			it = account_map->erase<1>(it);
		}

		// 再从数据库中删除
		strsql = "DELETE FROM League_LeagueInviteJoin WHERE league_uuid='";
		strsql += league_uuid.str();
		strsql += "';";
	}
	else
	{
		// 只删当前反馈的军团uuid和被邀请人的uuid相等的一条数据
		const auto range = account_map->equal_range<1>(league_uuid);
		for(auto it = range.first; it != range.second; ++it)
		{
			if(LeagueUuid(it->account->unlocked_get_league_uuid()) == league_uuid && LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid )
			{
				account_map->erase<1>(it);

				strsql = "DELETE FROM League_LeagueInviteJoin WHERE league_uuid='";
				strsql += league_uuid.str();
				strsql += "' AND legion_uuid='";
				strsql += legion_uuid.str();
				strsql += "';";

				break;

			}
		}
	}

	LOG_EMPERY_LEAGUE_DEBUG("deleteInfo_by_invitedres_uuid strsql = ", strsql);

	Poseidon::MySqlDaemon::enqueue_for_deleting("League_LeagueInviteJoin",strsql);
}

}
