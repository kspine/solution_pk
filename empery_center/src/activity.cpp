#include "precompiled.hpp"
#include "activity.hpp"
#include "data/activity.hpp"
#include "activity_ids.hpp"
#include "msg/sc_activity.hpp"
#include "singletons/world_map.hpp"
#include "map_event_block.hpp"


namespace EmperyCenter {

Activity::Activity(std::uint64_t unique_id_,std::uint64_t available_since_,std::uint64_t available_until_):
	m_unique_id(unique_id_),m_available_since(available_since_),m_available_until(available_until_)
	{
	}

void Activity::pump_status(){
}

MapActivity::MapActivity(std::uint64_t unique_id_,std::uint64_t available_since_,std::uint64_t available_until_)
	: Activity(unique_id_,available_since_,available_until_),m_current_activity(MapActivityId(0)),m_next_activity_time(0)
{
	std::vector<boost::shared_ptr<const Data::MapActivity>> map_activity_data;
	Data::MapActivity::get_all(map_activity_data);
	std::uint64_t  last_util_time = m_available_since;
	std::uint64_t  new_until_time = m_available_since;
	for(auto it = map_activity_data.begin(); it != map_activity_data.end(); it++){
		new_until_time = saturated_add(last_util_time, static_cast<std::uint64_t>((*it)->continued_time * 60000));
		MapActivityDetailInfo info;
		info.unique_id = (*it)->unique_id;
		info.available_since = last_util_time;
		info.available_until = new_until_time;
		last_util_time = new_until_time;
		m_activitys.emplace(MapActivityId(info.unique_id),std::move(info));
	}
}

void MapActivity::pump_status(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();
	if(m_current_activity == MapActivityId(ActivityIds::ID_MAP_ACTIVITY_END)){
		return;
	}
	if(utc_now < m_available_since || utc_now > m_available_until || utc_now < m_next_activity_time)
		return;

	const auto delta = utc_now - m_available_since;
	std::vector<boost::shared_ptr<const Data::MapActivity>> ret;
	Data::MapActivity::get_all(ret);
	std::uint64_t  continued_time = 0;
	for(auto it = ret.begin(); it != ret.end(); it++){
		continued_time = saturated_add(continued_time, static_cast<std::uint64_t>((*it)->continued_time * 60000));
		if(continued_time > delta){
			set_current_activity(MapActivityId((*it)->unique_id));
			m_next_activity_time = m_available_since + continued_time;
			return;
		}
	}
	set_current_activity(MapActivityId(ActivityIds::ID_MAP_ACTIVITY_END));
}

void MapActivity::set_current_activity(MapActivityId activity_id){
	if(m_current_activity != activity_id){
		on_activity_change(m_current_activity, activity_id);
		m_current_activity = activity_id;
	}
}

MapActivityId MapActivity::get_current_activity(){
	return m_current_activity;
}

void MapActivity::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const{
	Msg::SC_MapActivityInfo msg;
	for(auto it = m_activitys.begin(); it != m_activitys.end(); ++it){
		auto &activity = *msg.activitys.emplace(msg.activitys.end());
		activity.unique_id = (it->second).unique_id;
		activity.available_since = (it->second).available_since;
		activity.available_until = (it->second).available_until;
	}
	session->send(msg);
}

MapActivity::MapActivityDetailInfo MapActivity::get_activity_info(MapActivityId activityId){
	const auto it = m_activitys.find(activityId);
	if(it == m_activitys.end()){
		return { };
	}
	return it->second;
}

void MapActivity::on_activity_change(MapActivityId old_ativity,MapActivityId new_activity){
	if(old_ativity == ActivityIds::ID_MAP_ACTIVITY_RESOURCE){
		WorldMap::remove_activity_event(MapEventBlock::MAP_EVENT_ACTIVITY_RESOUCE);
	}
	if(old_ativity == ActivityIds::ID_MAP_ACTIVITY_GOBLIN){
		WorldMap::remove_activity_event(MapEventBlock::MAP_EVENT_ACTIVITY_GOBLIN);
	}
	if(new_activity == ActivityIds::ID_MAP_ACTIVITY_RESOURCE){
		WorldMap::refresh_activity_event(MapEventBlock::MAP_EVENT_ACTIVITY_RESOUCE);
	}
	if(new_activity == ActivityIds::ID_MAP_ACTIVITY_GOBLIN){
		WorldMap::refresh_activity_event(MapEventBlock::MAP_EVENT_ACTIVITY_GOBLIN);
	}
}

}
