#ifndef EMPERY_CENTER_SINGLETONS_MAP_ACTIVITY_ACCUMULATE_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_MAP_ACTIVITY_ACCUMULATE_MAP_HPP_

#include "../id_types.hpp"
#include <vector>
#include <cstdint>
#include "../coord.hpp"

namespace EmperyCenter {

class PlayerSession;

struct MapActivityAccumulateMap {
	using Progress = boost::container::flat_map<std::uint64_t, std::uint64_t>;
	struct AccumulateInfo {
		AccountUuid      account_uuid;
		MapActivityId    activity_id;
		std::uint64_t    avaliable_since;
		std::uint64_t    avaliable_util;
		std::uint64_t    accumulate_value;
		Progress         target_reward;
	};

	static AccumulateInfo get(AccountUuid account_uuid, MapActivityId activity_id,std::uint64_t since);
	static void get_recent(MapActivityId activity_id,std::uint64_t since,std::vector<AccumulateInfo> &ret);
	static void insert(AccumulateInfo info);
	static void update(AccumulateInfo info,bool throws_if_not_exists);
	static void synchronize_map_accumulate_info_with_player(AccumulateInfo info);
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
		bool             rewarded;
	};
	static void get_recent_rank_list(MapActivityId activity_id, std::uint64_t settle_date,std::vector<MapActivityRankInfo> &ret);
	static bool get_account_rank_info(MapActivityId activity_id, std::uint64_t settle_date,AccountUuid account_uuid,MapActivityRankInfo &info);
	static void insert(MapActivityRankInfo info);
	static void update(MapActivityRankInfo info);
private:
	MapActivityRankMap() = delete;
};

struct WorldActivityMap {
	struct WorldActivityInfo{
		WorldActivityId activity_id;
		Coord             cluster_coord;
		std::uint64_t     since;
		std::uint64_t     sub_since;
		std::uint64_t     sub_until;
		std::uint64_t     accumulate_value;
		bool              finish;
	};
	static WorldActivityInfo get(Coord coord, WorldActivityId activity_id,std::uint64_t since);
	static void get_recent_world_activity(Coord coord,std::uint64_t since,std::vector<WorldActivityInfo> &ret);
	static void insert(WorldActivityInfo info);
	static void update(WorldActivityInfo info,bool throws_if_not_exists);
private:
	WorldActivityMap() = delete;
};

struct WorldActivityAccumulateMap {
	struct WorldActivityAccumulateInfo{
		AccountUuid          account_uuid;
		WorldActivityId      activity_id;
		Coord                cluster_coord;
		std::uint64_t        since;
		std::uint64_t        accumulate_value;
		bool                 rewarded;
	};
	static WorldActivityAccumulateInfo get(AccountUuid account_uuid,Coord coord, WorldActivityId activity_id,std::uint64_t since);
	static void get_recent_activity_accumulate_info(Coord coord,std::uint64_t since,std::vector<WorldActivityAccumulateInfo> &ret);
	static void update(WorldActivityAccumulateInfo info);
	static void get_recent_world_activity_account(Coord coord, WorldActivityId activity_id,std::uint64_t since,std::vector<AccountUuid> &ret);
private:
	WorldActivityAccumulateMap() = delete;
};

struct WorldActivityRankMap {
	struct WorldActivityRankInfo{
		AccountUuid          account_uuid;
		Coord                cluster_coord;
		std::uint64_t        since;
		std::uint64_t        rank;
		std::uint64_t        accumulate_value;
		std::uint64_t        process_date;
		bool                 rewarded;
	};
	static WorldActivityRankInfo get_account_rank(AccountUuid account_uuid, Coord coord, std::uint64_t since);
	static void get_recent_rank_list(Coord coord, std::uint64_t since,std::uint64_t rank_threshold,std::vector<WorldActivityRankInfo> &ret);
	static void insert(WorldActivityRankInfo info);
	static void update(WorldActivityRankInfo info);
	static std::uint64_t get_pro_rank_time(Coord coord, std::uint64_t since);
private:
	WorldActivityRankMap() = delete;
};

struct WorldActivityBossMap {
	struct WorldActivityBossInfo{
		Coord                cluster_coord;
		std::uint64_t        since;
		MapObjectUuid        boss_uuid;
		std::uint64_t        create_date;
		std::uint64_t        delete_date;
		std::uint64_t        hp_total;
		std::uint64_t        hp_die;
	};
	static WorldActivityBossInfo get(Coord cluster_coord, std::uint64_t since);
	static void update(WorldActivityBossInfo info);
private:
	WorldActivityBossMap() = delete;
};

}

#endif
