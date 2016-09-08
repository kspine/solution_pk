#include "../precompiled.hpp"
#include "league_map.hpp"
#include "../mmain.hpp"
#include "../league.hpp"
#include "../string_utilities.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>


namespace EmperyLeague {

namespace {
	struct LeagueElement {
		boost::shared_ptr<League> league;

		LeagueUuid league_uuid;
		std::size_t nick_hash;
		LegionUuid legion_uuid;
		AccountUuid create_uuid;


		explicit LeagueElement(boost::shared_ptr<League> league_)
			: league(std::move(league_))
			, league_uuid(league->get_league_uuid())
			, nick_hash(hash_string_nocase(league->get_nick()))
			, legion_uuid(league->get_legion_uuid())
			, create_uuid(league->get_create_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(LeagueContainer, LeagueElement,
		UNIQUE_MEMBER_INDEX(league_uuid)
		MULTI_MEMBER_INDEX(legion_uuid)
		MULTI_MEMBER_INDEX(create_uuid)
		MULTI_MEMBER_INDEX(nick_hash)
	)

	boost::weak_ptr<LeagueContainer> g_league_map;

	MODULE_RAII_PRIORITY(handles, 5000){

		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// League
		struct TempLeagueElement {
			boost::shared_ptr<MySql::League_Info> obj;
			std::vector<boost::shared_ptr<MySql::League_LeagueAttribute>> attributes;
		};
		std::map<LeagueUuid, TempLeagueElement> temp_account_map;

		LOG_EMPERY_LEAGUE_INFO("Loading Leagues...");
		conn->execute_sql("SELECT * FROM `League_Info`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::League_Info>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			const auto league_uuid = LeagueUuid(obj->get_league_uuid());
			temp_account_map[league_uuid].obj = std::move(obj);
		}
		LOG_EMPERY_LEAGUE_INFO("Loaded ", temp_account_map.size(), " Leagues(s).");

		LOG_EMPERY_LEAGUE_INFO("Loading Leagues attributes...");
		conn->execute_sql("SELECT * FROM `League_LeagueAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::League_LeagueAttribute>();
			obj->fetch(conn);
			const auto league_uuid = LeagueUuid(obj->unlocked_get_league_uuid());
			const auto it = temp_account_map.find(league_uuid);
			if(it == temp_account_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.attributes.emplace_back(std::move(obj));
		}

		LOG_EMPERY_LEAGUE_INFO("Done loading Leagues attributes.");

		const auto league_map = boost::make_shared<LeagueContainer>();
		for(auto it = temp_account_map.begin(); it != temp_account_map.end(); ++it){
			auto league = boost::make_shared<League>(std::move(it->second.obj), it->second.attributes);

		//	LOG_EMPERY_LEAGUE_INFO("league attributes size==============================================",it->second.attributes.size());

		//	LOG_EMPERY_LEAGUE_INFO("league members size==============================================",it->second.members.size());

			league_map->insert(LeagueElement(std::move(league)));

		//	sleep(5000);
		}


		g_league_map = league_map;

	//	const auto league_map = boost::make_shared<LeagueContainer>();

		handles.push(league_map);
	}

	template<typename IteratorT>
	void really_append_league(std::vector<boost::shared_ptr<League>> &ret,
		const std::pair<IteratorT, IteratorT> &range, std::uint64_t begin, std::uint64_t count)
	{
		PROFILE_ME;

		auto lower = range.first;
		for(std::uint64_t i = 0; i < begin; ++i){
			if(lower == range.second){
				break;
			}
			++lower;
		}

		auto upper = lower;
		for(std::uint64_t i = 0; i < count; ++i){
			if(upper == range.second){
				break;
			}
			++upper;
		}

		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(lower, upper)));
		for(auto it = lower; it != upper; ++it){
			ret.emplace_back(it->league);
		}
	}
}

boost::shared_ptr<League> LeagueMap::get(LeagueUuid league_uuid){
	PROFILE_ME;

	const auto league_map = g_league_map.lock();
	if(!league_map){
		LOG_EMPERY_LEAGUE_WARNING("leagueMap is not loaded.");
		return { };
	}

	const auto it = league_map->find<0>(league_uuid);
	if(it == league_map->end<0>()){
		LOG_EMPERY_LEAGUE_DEBUG("league not found: league_uuid = ", league_uuid);
		return { };
	}
	return it->league;
}
boost::shared_ptr<League> LeagueMap::require(LeagueUuid league_uuid){
	PROFILE_ME;

	auto league = get(league_uuid);
	if(!league){
		LOG_EMPERY_LEAGUE_WARNING("league not found: league_uuid = ", league_uuid);
		DEBUG_THROW(Exception, sslit("league not found"));
	}
	return league;
}
void LeagueMap::get_by_founder(std::vector<boost::shared_ptr<League>> &ret, AccountUuid account_uuid){
	PROFILE_ME;

	const auto league_map = g_league_map.lock();
	if(!league_map){
		LOG_EMPERY_LEAGUE_WARNING("LeagueMap is not loaded.");
		return;
	}

	const auto range = league_map->equal_range<2>(account_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->league);
	}
}

void LeagueMap::get_by_nick(std::vector<boost::shared_ptr<League>> &ret, const std::string &nick)
{
	PROFILE_ME;
	const auto league_map = g_league_map.lock();
	if(!league_map){
		LOG_EMPERY_LEAGUE_WARNING("LeagueMap is not loaded.");
		return;
	}

	const auto key = hash_string_nocase(nick);
	const auto range = league_map->equal_range<3>(key);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(!are_strings_equal_nocase(it->league->get_nick(), nick)){
			continue;
		}
		ret.emplace_back(it->league);
	}
}

void LeagueMap::get_by_legion_uuid(std::vector<boost::shared_ptr<League>> &ret,LegionUuid legion_uuid)
{
	PROFILE_ME;

	const auto league_map = g_league_map.lock();
	if(!league_map){
		LOG_EMPERY_LEAGUE_WARNING("leagueMap is not loaded.");
		return;
	}

	const auto range = league_map->equal_range<1>(legion_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->league);
	}
}

void LeagueMap::insert(const boost::shared_ptr<League> &league){
	PROFILE_ME;

	const auto league_map = g_league_map.lock();
	if(!league_map){
		LOG_EMPERY_LEAGUE_WARNING("LeagueMap is not loaded.");
		DEBUG_THROW(Exception, sslit("LeagueMap is not loaded"));
	}

	const auto league_uuid = league->get_league_uuid();

	LOG_EMPERY_LEAGUE_DEBUG("Inserting league: league_uuid = ", league_uuid);
	if(!league_map->insert(LeagueElement(league)).second){
		LOG_EMPERY_LEAGUE_WARNING("league already exists: league_uuid = ", league_uuid);
		DEBUG_THROW(Exception, sslit("league already exists"));
	}
}
void LeagueMap::update(const boost::shared_ptr<League> &league, bool throws_if_not_exists){
	PROFILE_ME;

	const auto league_map = g_league_map.lock();
	if(!league_map){
		LOG_EMPERY_LEAGUE_WARNING("LeagueMap is not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("LeagueMap is not loaded"));
		}
		return;
	}

	const auto league_uuid = league->get_league_uuid();

	const auto it = league_map->find<0>(league_uuid);
	if(it == league_map->end<0>()){
		LOG_EMPERY_LEAGUE_DEBUG("league not found: league_uuid = ", league_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("league not found"));
		}
		return;
	}
	if(it->league != league){
		LOG_EMPERY_LEAGUE_DEBUG("league expired: league_uuid = ", league_uuid);
		return;
	}

	LOG_EMPERY_LEAGUE_DEBUG("Updating league: league_uuid = ", league_uuid);
	league_map->replace<0>(it, LeagueElement(league));
}


void LeagueMap::remove(LeagueUuid league_uuid){
	PROFILE_ME;

	const auto league_map = g_league_map.lock();
	if(!league_map){
		LOG_EMPERY_LEAGUE_WARNING("League map not loaded.");
		return;
	}

	const auto it = league_map->find<0>(league_uuid);
	if(it != league_map->end<0>()){

		const auto& league = it->league;

		// 联盟解散的善后操作
		league->disband();

		// 先从内存中删除
		LOG_EMPERY_LEAGUE_TRACE("Removing League: league_uuid = ", league_uuid);
		league_map->erase<0>(it);

		// 从数据库中删掉
		std::string strsql = "DELETE FROM League_Info WHERE league_uuid='";
		strsql += league_uuid.str();
		strsql += "';";

		Poseidon::MySqlDaemon::enqueue_for_deleting("League_Info",strsql);
	}
}

void LeagueMap::get_all(std::vector<boost::shared_ptr<League>> &ret, std::uint64_t begin, std::uint64_t count)
{
	PROFILE_ME;

	const auto league_map = g_league_map.lock();
	if(!league_map){
		LOG_EMPERY_LEAGUE_WARNING("League map not loaded.");
		return;
	}

	really_append_league(ret,
		std::make_pair(league_map->begin<0>(), league_map->end<0>()),
		begin, count);
}

}
