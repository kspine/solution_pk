#ifndef EMPERY_LEAGUE_SINGLETONS_LEAGUE_INVITEJOIN_MAP_HPP_
#define EMPERY_LEAGUE_SINGLETONS_LEAGUE_INVITEJOIN_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyLeague {


namespace MySql {
	class League_LeagueInviteJoin;
}

class LeagueSession;


struct LeagueInviteJoinMap {

	static boost::shared_ptr<MySql::League_LeagueInviteJoin> find(LeagueUuid league_uuid,LegionUuid legion_uuid);

	static void get_by_invited_uuid(std::vector<boost::shared_ptr<MySql::League_LeagueInviteJoin>> &ret, LeagueUuid league_uuid);

	static boost::shared_ptr<MySql::League_LeagueInviteJoin> find_inviteinfo_by_user(LeagueUuid league_uuid,LegionUuid legion_uuid);

	static std::uint64_t get_apply_count(LegionUuid legion_uuid);

	static void insert(const boost::shared_ptr<MySql::League_LeagueInviteJoin> &account);
	static void deleteInfo(LegionUuid legion_uuid,LeagueUuid league_uuid,bool bAll = false);

	static void deleteInfo_by_invited_uuid(LeagueUuid league_uuid);
	static void deleteInfo_by_legion_uuid(LegionUuid legion_uuid);
	static void deleteInfo_by_invitedres_uuid(LeagueUuid league_uuid,LegionUuid legion_uuid,bool bAll = false);

	static void synchronize_with_player(const boost::shared_ptr<LeagueSession>& league_client,LegionUuid legion_uuid,AccountUuid account_uuid);

private:
	LeagueInviteJoinMap() = delete;
};

}

#endif
