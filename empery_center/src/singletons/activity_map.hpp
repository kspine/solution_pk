#ifndef EMPERY_CENTER_SINGLETONS_ACTIVITY_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_ACTIVITY_MAP_HPP_

#include "../id_types.hpp"
#include "../coord.hpp"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace EmperyCenter {

class Activity;
class MapActivity;
class WorldActivity;

struct ActivityMap {
	static boost::shared_ptr<Activity> get(std::uint64_t unique_id);
	static boost::shared_ptr<Activity> require(std::uint64_t unique_id);
	static boost::shared_ptr<MapActivity> get_map_activity();
	static boost::shared_ptr<WorldActivity> get_world_activity();
private:
	ActivityMap() = delete;
};

}

#endif
