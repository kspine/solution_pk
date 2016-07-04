#include "../precompiled.hpp"
#include "activity_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include "../activity.hpp"
#include "../data/activity.hpp"
#include "../activity_ids.hpp"

namespace EmperyCenter {

namespace {

	struct ActivityElement {
		boost::shared_ptr<Activity> activity;
		std::uint64_t unique_id;
		explicit ActivityElement(boost::shared_ptr<Activity> activity_)
			: activity(std::move(activity_))
			, unique_id(activity->m_unique_id)
		{
		}
	};

	MULTI_INDEX_MAP(ActivityContainer, ActivityElement,
		UNIQUE_MEMBER_INDEX(unique_id)
	)

	boost::weak_ptr<ActivityContainer> g_activity_map;

	void refresh_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Map event block refresh timer: now = ", now);

		const auto activity_map = g_activity_map.lock();
		if(!activity_map){
			return;
		}

		for(auto it = activity_map->begin<0>(); it != activity_map->end<0>(); ++it){
			const auto &activity = it->activity;

			activity->pump_status();
		}
	}

	boost::shared_ptr<Activity> create_activity(std::uint64_t unique_id, std::uint64_t available_since,std::uint64_t available_until){
		boost::shared_ptr<Activity> activity;
		switch(unique_id){
			case ActivityIds::ID_MAP_ACTIVITY.get():
				activity = boost::make_shared<MapActivity>(unique_id, available_since,available_until);
				break;
			case ActivityIds::ID_WORLD_ACTIVITY.get():
				activity = boost::make_shared<MapActivity>(unique_id, available_since,available_until);
				break;
			default:
				LOG_EMPERY_CENTER_DEBUG("unknow activity: ", unique_id);
				DEBUG_THROW(Exception, sslit("unknow activity:"));
				break;
		}
		return activity;
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto activity_map = boost::make_shared<ActivityContainer>();
		std::vector<boost::shared_ptr<const Data::Activity>> ret;
		Data::Activity::get_all(ret);
		for(auto it = ret.begin(); it != ret.end(); ++it){
			auto unique_id = (*it)->unique_id;
			auto available_since = (*it)->available_since;
			auto available_until = (*it)->available_until;
			auto activity = create_activity(unique_id,available_since,available_until);
			activity_map->insert(ActivityElement(std::move(activity)));
		}
		g_activity_map = activity_map;
		handles.push(activity_map);

		const auto refresh_interval = get_config<std::uint64_t>("map_event_refresh_interval", 60000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, refresh_interval,
			std::bind(&refresh_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<Activity> ActivityMap::get(std::uint64_t unique_id){
	PROFILE_ME;
	const auto activity_map = g_activity_map.lock();
	if(!activity_map){
		LOG_EMPERY_CENTER_WARNING("ActivityMap has not been loaded.");
		return { };
	}

	const auto it = activity_map->find<0>(unique_id);
	if(it == activity_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("ActivityMap not found: unique_id = ", unique_id);
		return { };
	}
	return  it->activity;
}
boost::shared_ptr<Activity> ActivityMap::require(std::uint64_t unique_id){
	PROFILE_ME;
	auto ret = get(unique_id);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("ActivityMap not found: unique_id = ", unique_id);;
		DEBUG_THROW(Exception, sslit("ActivityMap not found"));
	}
	return ret;
}

boost::shared_ptr<MapActivity> ActivityMap::get_map_activity(){
	PROFILE_ME;
	auto activity = require(ActivityIds::ID_MAP_ACTIVITY.get());
	return boost::dynamic_pointer_cast<MapActivity>(activity);
}

}
