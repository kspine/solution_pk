#ifndef EMPERY_CENTER_SINGLETONS_MAP_ACTIVITY_ACCUMULATE_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_MAP_ACTIVITY_ACCUMULATE_MAP_HPP_

#include "../id_types.hpp"
#include <vector>
#include <cstdint>

namespace EmperyCenter {

class PlayerSession;

struct MapActivityAccumulateMap {
	struct AccumulateInfo {
		AccountUuid      account_uuid;
		MapActivityId    activity_id;
		std::uint64_t    avaliable_since;
		std::uint64_t    avaliable_util;
		std::uint64_t    accumulate_value;
	};

	static AccumulateInfo get(AccountUuid account_uuid, MapActivityId activity_id);
	static void insert(AccumulateInfo info);
	static void update(AccumulateInfo info,bool throws_if_not_exists);
private:
	MapActivityAccumulateMap() = delete;
};

struct MapActivityRankMap {
	struct MapActivityRankInfo {
		AccountUuid      account_uuid;
		MapActivityId    activity_id;
		std::uint64_t    settle_date;
		std::uint64_t    rank;
		std::uint64_t    accumulate_value;
		std::uint64_t    process_date;
	};
	static void get_recent_rank_list(MapActivityId activity_id, std::uint64_t settle_date,std::vector<MapActivityRankInfo> &ret);
	static void insert(MapActivityRankInfo info);
};

}

#endif
