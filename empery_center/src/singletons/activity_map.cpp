#include "../precompiled.hpp"
#include "activity_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/json.hpp>
#include "../activity.hpp"
#include "../data/activity.hpp"
#include "../activity_ids.hpp"
#include "../mysql/activity_config.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>

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

	MODULE_RAII_PRIORITY(handles, 5000){

		const auto activity_map = boost::make_shared<ActivityContainer>();
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

	if(activity_map->empty()){
		reload();
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

boost::shared_ptr<WorldActivity> ActivityMap::get_world_activity(){
	PROFILE_ME;

	auto activity = require(ActivityIds::ID_WORLD_ACTIVITY.get());
	return boost::dynamic_pointer_cast<WorldActivity>(activity);
}

void  ActivityMap::reload(){
	PROFILE_ME;

	Data::Activity::reload();
	Data::MapActivity::reload();
	Data::WorldActivity::reload();
	Data::ActivityAward::reload();
	const auto activity_map = g_activity_map.lock();
	if(!activity_map){
		LOG_EMPERY_CENTER_WARNING("ActivityMap has not been loaded.");
		return;
	}
	activity_map->clear();
	std::vector<boost::shared_ptr<const Data::Activity>> ret;
	Data::Activity::get_all(ret);
	for(auto it = ret.begin(); it != ret.end(); ++it){
		auto unique_id = (*it)->unique_id;
		auto available_since = (*it)->available_since;
		auto available_until = (*it)->available_until;
		boost::shared_ptr<Activity> activity;
		switch(unique_id){
			case ActivityIds::ID_MAP_ACTIVITY.get():
				activity = boost::make_shared<MapActivity>(unique_id, available_since,available_until);
				break;
			case ActivityIds::ID_WORLD_ACTIVITY.get():
				activity = boost::make_shared<WorldActivity>(unique_id, available_since,available_until);
				break;
			default:
				LOG_EMPERY_CENTER_DEBUG("unknow activity: ", unique_id);
				continue;
				break;
		}
		activity_map->insert(ActivityElement(std::move(activity)));
	}
}

void  ActivityMap::force_load_activitys(Poseidon::JsonArray &activitys){
	PROFILE_ME;

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_Activitys>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_Activitys` order by activity_id ASC";
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Center_Activitys>();
				obj->fetch(conn);
				sink->emplace_back(std::move(obj));
			}, "Center_Activitys", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		LOG_EMPERY_CENTER_WARNING("activitys not found in database ");
	}
	for(auto it = sink->begin(); it != sink->end(); ++it){
		auto &obj = *it;
		Poseidon::JsonArray activity;
		activity.emplace_back(obj->get_activity_id());
		activity.emplace_back(obj->get_begin_time());
		activity.emplace_back(obj->get_end_time());
		activitys.emplace_back(activity);
	}
}
void  ActivityMap::force_load_activitys_map(Poseidon::JsonArray &activitys_map){
	PROFILE_ME;

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_ActivitysMap>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_ActivitysMap` order by activity_id ASC";
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Center_ActivitysMap>();
				obj->fetch(conn);
				sink->emplace_back(std::move(obj));
			}, "Center_ActivitysMap", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		LOG_EMPERY_CENTER_WARNING("activity map not found in database ");
	}
	for(auto it = sink->begin(); it != sink->end(); ++it){
		auto &obj = *it;
		Poseidon::JsonArray activity;
		activity.emplace_back(obj->get_activity_id());
		activity.emplace_back(obj->get_type());
		activity.emplace_back(obj->get_duration());
		activity.emplace_back(obj->get_target());
		activitys_map.emplace_back(activity);
	}
}
void  ActivityMap::force_load_activitys_world(Poseidon::JsonArray &activitys_world){
	PROFILE_ME;

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_ActivitysWorld>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_ActivitysWorld` order by activity_id ASC";
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Center_ActivitysWorld>();
				obj->fetch(conn);
				sink->emplace_back(std::move(obj));
			}, "Center_ActivitysWorld", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		LOG_EMPERY_CENTER_WARNING("activity map not found in database ");
	}
	for(auto it = sink->begin(); it != sink->end(); ++it){
		auto &obj = *it;
		Poseidon::JsonArray activity;
		activity.emplace_back(obj->get_activity_id());
		activity.emplace_back(obj->get_pro_activity_id());
		activity.emplace_back(obj->get_own_activity_id());
		activity.emplace_back(obj->get_target());
		activity.emplace_back(obj->get_type());
		activity.emplace_back(obj->get_drop());
		activitys_world.emplace_back(activity);
	}
}
void  ActivityMap::force_load_activitys_rank_award(Poseidon::JsonArray &rank_awards){
	PROFILE_ME;

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_ActivitysRankAward>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_ActivitysRankAward`";
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Center_ActivitysRankAward>();
				obj->fetch(conn);
				sink->emplace_back(std::move(obj));
			}, "Center_ActivitysRankAward", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		LOG_EMPERY_CENTER_WARNING("activity map not found in database ");
	}
	for(auto it = sink->begin(); it != sink->end(); ++it){
		auto &obj = *it;
		Poseidon::JsonArray reward;
		reward.emplace_back(obj->get_id());
		reward.emplace_back(obj->get_activity_id());
		Poseidon::JsonArray ranks;
		ranks.emplace_back(obj->get_rank_begin());
		ranks.emplace_back(obj->get_rank_end());
		reward.emplace_back(ranks);
		reward.emplace_back(obj->get_reward());
		rank_awards.emplace_back(reward);
	}

}

void  ActivityMap::force_update_activitys(std::uint64_t activity_id,std::string name,std::uint64_t begin_time,std::uint64_t end_time){
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::Center_Activitys>(activity_id,name,begin_time,end_time);
	const auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj,true,true);
	Poseidon::JobDispatcher::yield(promise, true);
}

void  ActivityMap::force_update_activitys_map(std::uint64_t activity_id,std::string name,std::uint32_t type,std::uint32_t duration,std::string target){
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::Center_ActivitysMap>(activity_id,name,type,duration,target);
	const auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj,true,true);
	Poseidon::JobDispatcher::yield(promise, true);
}
void  ActivityMap::force_update_activitys_world(std::uint64_t activity_id,std::string name,std::uint64_t pro_activity_id,std::uint64_t own_activity_id,std::uint32_t type,std::string target,std::string drop){
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::Center_ActivitysWorld>(activity_id,name,pro_activity_id,own_activity_id,type,target,drop);
	const auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj,true,true);
	Poseidon::JobDispatcher::yield(promise, true);
}
void  ActivityMap::force_update_activitys_rank_award(std::uint64_t id,std::uint64_t activity_id,std::uint32_t rank_begin,std::uint32_t rank_end,std::string reward){
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::Center_ActivitysRankAward>(id,activity_id,rank_begin,rank_end,reward);
	const auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj,true,true);
	Poseidon::JobDispatcher::yield(promise, true);
}

void  ActivityMap::force_delete_activitys(std::uint64_t activity_id){
	PROFILE_ME;

	std::ostringstream oss;
	oss <<"DELETE FROM `Center_Activitys` WHERE `activity_id` = " <<activity_id;
	const auto promise = Poseidon::MySqlDaemon::enqueue_for_deleting("Center_Activitys",oss.str());
	Poseidon::JobDispatcher::yield(promise, true);
}
void  ActivityMap::force_delete_activitys_map(std::uint64_t activity_id){
	PROFILE_ME;

	std::ostringstream oss;
	oss <<"DELETE FROM `Center_ActivitysMap` WHERE `activity_id` = " <<activity_id;
	const auto promise = Poseidon::MySqlDaemon::enqueue_for_deleting("Center_ActivitysMap",oss.str());
	Poseidon::JobDispatcher::yield(promise, true);
}
void  ActivityMap::force_delete_activitys_world(std::uint64_t activity_id){
	PROFILE_ME;

	std::ostringstream oss;
	oss <<"DELETE FROM `Center_ActivitysWorld` WHERE `activity_id` = " <<activity_id;
	const auto promise = Poseidon::MySqlDaemon::enqueue_for_deleting("Center_ActivitysWorld",oss.str());
	Poseidon::JobDispatcher::yield(promise, true);
}
void  ActivityMap::force_delete_activitys_rank_award(std::uint64_t id){
	PROFILE_ME;

	std::ostringstream oss;
	oss <<"DELETE FROM `Center_ActivitysRankAward` WHERE `id` = " <<id;
	const auto promise = Poseidon::MySqlDaemon::enqueue_for_deleting("Center_ActivitysRankAward",oss.str());
	Poseidon::JobDispatcher::yield(promise, true);
}

}
