#ifndef EMPERY_CENTER_SINGLETONS_ACTIVITY_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_ACTIVITY_MAP_HPP_

#include "../id_types.hpp"
#include "../coord.hpp"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <poseidon/json.hpp>

namespace EmperyCenter {

class Activity;
class MapActivity;
class WorldActivity;

struct ActivityMap {
	static boost::shared_ptr<Activity> get(std::uint64_t unique_id);
	static boost::shared_ptr<Activity> require(std::uint64_t unique_id);
	static boost::shared_ptr<MapActivity> get_map_activity();
	static boost::shared_ptr<WorldActivity> get_world_activity();
	static void  reload();
	static void  force_load_activitys(Poseidon::JsonArray &activitys);
	static void  force_load_activitys_map(Poseidon::JsonArray &activitys_map);
	static void  force_load_activitys_world(Poseidon::JsonArray &activitys_world);
	static void  force_load_activitys_rank_award(Poseidon::JsonArray &rank_award);
	static void  force_update_activitys(std::uint64_t activity_id,std::string name,std::uint64_t begin_time,std::uint64_t end_time);
	static void  force_update_activitys_map(std::uint64_t activity_id,std::string name,std::uint32_t type,std::uint32_t duration,std::string target);
	static void  force_update_activitys_world(std::uint64_t activity_id,std::string name,std::uint64_t pro_activity_id,std::uint64_t own_activity_id,std::uint32_t type,std::string target,std::string drop);
	static void  force_update_activitys_rank_award(std::uint64_t id,std::uint64_t activity_id,std::uint32_t rank_begin,std::uint32_t rank_end,std::string reward);
	static void  force_delete_activitys(std::uint64_t activity_id);
	static void  force_delete_activitys_map(std::uint64_t activity_id);
	static void  force_delete_activitys_world(std::uint64_t activity_id);
	static void  force_delete_activitys_rank_award(std::uint64_t id);
private:
	ActivityMap() = delete;
};

}

#endif
