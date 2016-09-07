#ifndef EMPERY_LEAGUE_SINGLETONS_LEAGUE_APPLYJOIN_MAP_HPP_
#define EMPERY_LEAGUE_SINGLETONS_LEAGUE_APPLYJOIN_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyLeague {


namespace MySql {
	class League_LeagueApplyJoin;
}

class LeagueSession;


struct LeagueApplyJoinMap {

	static boost::shared_ptr<MySql::League_LeagueApplyJoin> find(LegionUuid legion_uuid,LeagueUuid league_uuid);

	static std::uint64_t get_apply_count(LegionUuid legion_uuid);

	static void insert(const boost::shared_ptr<MySql::League_LeagueApplyJoin> &account);
	static void deleteInfo(LegionUuid legion_uuid,LeagueUuid league_uuid,bool bAll = false);
	static void deleteInfo_by_legion_uuid(LegionUuid legion_uuid);

	static void synchronize_with_player(const boost::shared_ptr<LeagueSession>& league_client,LeagueUuid league_uuid, std::string str_account_uuid);

private:
	LeagueApplyJoinMap() = delete;
};

}

#endif
