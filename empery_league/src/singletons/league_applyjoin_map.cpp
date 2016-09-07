#include "../precompiled.hpp"
#include "league_applyjoin_map.hpp"
#include "../mmain.hpp"
#include "../string_utilities.hpp"
#include "../mysql/league.hpp"
#include "../../../empery_center/src/msg/ls_league.hpp"
#include "../league_session.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <tuple>


namespace EmperyLeague {

namespace {
	struct LegionElement {
		boost::shared_ptr<MySql::League_LeagueApplyJoin> account;

		LegionUuid legion_uuid;
		LeagueUuid league_uuid;

		explicit LegionElement(boost::shared_ptr<MySql::League_LeagueApplyJoin> account_)
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

	boost::shared_ptr<LegionContainer> g_applyjoin_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// Legion
		struct TempLegionElement {
			boost::shared_ptr<MySql::League_LeagueApplyJoin> obj;
		};
		std::map<LegionUuid, TempLegionElement> temp_account_map;

		LOG_EMPERY_LEAGUE_INFO("Loading League_LeagueApplyJoin...");
		conn->execute_sql("SELECT * FROM `League_LeagueApplyJoin`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::League_LeagueApplyJoin>();
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
		g_applyjoin_map = legion_map;
		handles.push(legion_map);

	}

}


void LeagueApplyJoinMap::insert(const boost::shared_ptr<MySql::League_LeagueApplyJoin> &legion){
	PROFILE_ME;

	const auto &legion_map = g_applyjoin_map;
	if(!legion_map){
		LOG_EMPERY_LEAGUE_WARNING("LeagueApplyJoinMap  not loaded.");
		DEBUG_THROW(Exception, sslit("LeagueApplyJoinMap not loaded"));
	}


	const auto legion_uuid = legion->get_legion_uuid();

	LOG_EMPERY_LEAGUE_DEBUG("Inserting legion: legion_uuid = ", legion_uuid);

	if(!legion_map->insert(LegionElement(legion)).second){
		LOG_EMPERY_LEAGUE_WARNING("Legion already exists: legion_uuid = ", legion_uuid);
		DEBUG_THROW(Exception, sslit("Legion already exists"));
	}
}


boost::shared_ptr<MySql::League_LeagueApplyJoin> LeagueApplyJoinMap::find(LegionUuid legion_uuid,LeagueUuid league_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_applyjoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("legion map not loaded.");
		return { };
	}


	const auto range = account_map->equal_range<0>(legion_uuid);
	for(auto it = range.first; it != range.second; ++it){
		if(LeagueUuid(it->account->unlocked_get_league_uuid()) == league_uuid && LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid)
		{
			return it->account;
		}
	}


	return { };
}

std::uint64_t LeagueApplyJoinMap::get_apply_count(LegionUuid legion_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_applyjoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("legion map not loaded.");
		return { };
	}

	const auto range = account_map->equal_range<0>(legion_uuid);

	return static_cast<std::uint64_t>(std::distance(range.first, range.second));
}

void LeagueApplyJoinMap::synchronize_with_player(const boost::shared_ptr<LeagueSession>& league_client,LeagueUuid league_uuid, std::string str_account_uuid)
{
	PROFILE_ME;

	const auto &account_map = g_applyjoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("legion map not loaded.");
		EmperyCenter::Msg::LS_ApplyJoinList msg;
		msg.account_uuid = str_account_uuid;
		msg.applies.reserve(0);
		league_client->send(msg);
		return ;
	}

	EmperyCenter::Msg::LS_ApplyJoinList msg;
	msg.account_uuid = str_account_uuid;
	const auto range = account_map->equal_range<1>(league_uuid);
	msg.applies.reserve(static_cast<std::uint64_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it)
	{
		auto &elem = *msg.applies.emplace(msg.applies.end());
		const auto &info = it->account;
		elem.legion_uuid = LegionUuid(info->unlocked_get_legion_uuid()).str();
	}

	LOG_EMPERY_LEAGUE_WARNING("league applies count:",msg.applies.size());

	league_client->send(msg);
}

void LeagueApplyJoinMap::deleteInfo(LegionUuid legion_uuid,LeagueUuid league_uuid,bool bAll /*= false*/)
{
	PROFILE_ME;
	const auto &account_map = g_applyjoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("legion apply join map not loaded.");
		return;
	}

	std::string strsql = "";
	if(bAll)
	{
		// 全部删除该军团的数据,先删除内存中的,因为访问的是内存中的数据
		const auto range = account_map->equal_range<0>(legion_uuid);
		for(auto it = range.first; it != range.second;){
			it = account_map->erase<0>(it);
		}


		// 再删除数据库中的
		strsql = "DELETE FROM League_LeagueApplyJoin WHERE legion_uuid='";
		strsql += legion_uuid.str();
		strsql += "';";

	}
	else
	{
		const auto range = account_map->equal_range<0>(legion_uuid);
		for(auto it = range.first; it != range.second; ++it)
		{
			if(LeagueUuid(it->account->unlocked_get_league_uuid()) == league_uuid && LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid)
			{
				account_map->erase<0>(it);
				strsql = "DELETE FROM League_LeagueApplyJoin WHERE league_uuid='";
				strsql += league_uuid.str();
				strsql += "' AND legion_uuid='";
				strsql += legion_uuid.str();
				strsql += "';";

				break;

			}
		}
	}

	LOG_EMPERY_LEAGUE_DEBUG("Reclaiming league apply join map strsql = ", strsql);

	Poseidon::MySqlDaemon::enqueue_for_deleting("League_LeagueApplyJoin",strsql);
}

void LeagueApplyJoinMap::deleteInfo_by_legion_uuid(LegionUuid legion_uuid)
{
	PROFILE_ME;

	const auto &account_map = g_applyjoin_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("legion apply join map not loaded.");
		return;
	}

	// 全部删除该军团的数据,先删除内存中的,因为访问的是内存中的数据
	const auto range = account_map->equal_range<0>(legion_uuid);
	for(auto it = range.first; it != range.second;){
		it = account_map->erase<0>(it);
	}


	// 再删除数据库中的
	std::string strsql = "DELETE FROM League_LeagueApplyJoin WHERE legion_uuid='";
	strsql += legion_uuid.str();
	strsql += "';";


	Poseidon::MySqlDaemon::enqueue_for_deleting("League_LeagueApplyJoin",strsql);

}


}
