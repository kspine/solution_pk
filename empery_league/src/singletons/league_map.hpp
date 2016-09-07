#ifndef EMPERY_LEAGUE_SINGLETONS_LEAGUE_MAP_HPP_
#define EMPERY_LEAGUE_SINGLETONS_LEAGUE_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include <vector>
#include "../id_types.hpp"

namespace EmperyLeague {

class League;

struct LeagueMap {
	static boost::shared_ptr<League> get(LeagueUuid league_uuid);
	static boost::shared_ptr<League> require(LeagueUuid league_uuid);
	static void get_by_founder(std::vector<boost::shared_ptr<League>> &ret, AccountUuid founder_uuid);
	static void get_by_nick(std::vector<boost::shared_ptr<League>> &ret, const std::string &nick);

	static void get_by_legion_uuid(std::vector<boost::shared_ptr<League>> &ret,LegionUuid legion_uuid);

	static void insert(const boost::shared_ptr<League> &league);
	static void update(const boost::shared_ptr<League> &league, bool throws_if_not_exists = true);
	static void remove(LeagueUuid league_uuid) noexcept;

	static void get_all(std::vector<boost::shared_ptr<League>> &ret, std::uint64_t begin, std::uint64_t count);

private:
	LeagueMap() = delete;
};

}

#endif
