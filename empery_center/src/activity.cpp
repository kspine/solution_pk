#include "precompiled.hpp"
#include "activity.hpp"
#include "data/activity.hpp"
#include "activity_ids.hpp"
#include "msg/sc_activity.hpp"
#include "singletons/world_map.hpp"
#include "singletons/map_activity_accumulate_map.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "map_event_block.hpp"
#include "reason_ids.hpp"
#include "singletons/item_box_map.hpp"
#include "item_box.hpp"
#include "mysql/map_activity_accumulate.hpp"
#include "transaction_element.hpp"


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
	set_current_activity(MapActivityId(0));
}

void MapActivity::set_current_activity(MapActivityId activity_id){
	PROFILE_ME;

	if(m_current_activity != activity_id){
		on_activity_change(m_current_activity, activity_id);
		m_current_activity = activity_id;
	}
}

MapActivityId MapActivity::get_current_activity(){
	PROFILE_ME;

	return m_current_activity;
}

void MapActivity::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const{
	PROFILE_ME;

	Msg::SC_MapActivityInfo msg;
	msg.curr_unique_id  = m_current_activity.get();
	for(auto it = m_activitys.begin(); it != m_activitys.end(); ++it){
		auto &activity = *msg.activitys.emplace(msg.activitys.end());
		activity.unique_id = (it->second).unique_id;
		activity.available_since = (it->second).available_since;
		activity.available_until = (it->second).available_until;
	}
	session->send(msg);
}

MapActivity::MapActivityDetailInfo MapActivity::get_activity_info(MapActivityId activityId){
	PROFILE_ME;

	const auto it = m_activitys.find(activityId);
	if(it == m_activitys.end()){
		return { };
	}
	return it->second;
}

void MapActivity::on_activity_change(MapActivityId old_ativity,MapActivityId new_activity){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();
	if(old_ativity == ActivityIds::ID_MAP_ACTIVITY_RESOURCE){
		WorldMap::remove_activity_event(MapEventBlock::MAP_EVENT_ACTIVITY_RESOUCE);
	}
	if(old_ativity == ActivityIds::ID_MAP_ACTIVITY_GOBLIN){
		WorldMap::remove_activity_event(MapEventBlock::MAP_EVENT_ACTIVITY_GOBLIN);
	}
	if(old_ativity == ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER){
		settle_kill_soliders_activity(utc_now);
	}
	if(new_activity == ActivityIds::ID_MAP_ACTIVITY_RESOURCE){
		WorldMap::refresh_activity_event(MapEventBlock::MAP_EVENT_ACTIVITY_RESOUCE);
	}
	if(new_activity == ActivityIds::ID_MAP_ACTIVITY_GOBLIN){
		WorldMap::refresh_activity_event(MapEventBlock::MAP_EVENT_ACTIVITY_GOBLIN);
	}
}

bool  MapActivity::settle_kill_soliders_activity(std::uint64_t now){
	PROFILE_ME;

	MapActivity::MapActivityDetailInfo activity_kill_solider_info =  get_activity_info(MapActivityId(ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER));
	if(activity_kill_solider_info.unique_id != ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER.get()|| now < activity_kill_solider_info.available_until){
		return false;
	}
	std::vector<MapActivityRankMap::MapActivityRankInfo> ret;
	MapActivityRankMap::get_recent_rank_list(MapActivityId(ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER),activity_kill_solider_info.available_until,ret);
	if(!ret.empty()){
		return false;
	}
	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapActivityAccumulate>>>();
	{
		std::ostringstream oss;
		char str[256];
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<std::uint64_t>(activity_kill_solider_info.available_since), false);
		oss <<"SELECT * FROM `Center_MapActivityAccumulate` WHERE `map_activity_id` = " << ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER.get() << " and `avaliable_since` = " << Poseidon::MySql::StringEscaper(str) << " order by accumulate_value desc limit 50";
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Center_MapActivityAccumulate>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink->emplace_back(std::move(obj));
			}, "Center_MapActivityAccumulate", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		return false;
	}
	std::uint64_t rank = 1;
	for(auto it = sink->begin(); it != sink->end(); ++it,++rank){
		MapActivityRankMap::MapActivityRankInfo info = {};
		info.account_uuid     = AccountUuid((*it)->get_account_uuid());
		info.activity_id      = MapActivityId((*it)->get_map_activity_id());
		info.settle_date      = activity_kill_solider_info.available_until;
		info.rank             = rank;
		info.accumulate_value = (*it)->get_accumulate_value();
		info.process_date     = now;
		MapActivityRankMap::insert(std::move(info));
		//发送奖励
		try{
			std::vector<std::pair<std::uint64_t,std::uint64_t>> rewards;
			bool is_award_data = Data::ActivityAward::get_activity_rank_award(ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER.get(),rank,rewards);
			if(!is_award_data){
				LOG_EMPERY_CENTER_WARNING("ACTIVITY_KILL_SOLDIER has no award data, rank:",rank);
				continue;
			}
			const auto item_box = ItemBoxMap::get(info.account_uuid);
			if(!item_box){
				LOG_EMPERY_CENTER_WARNING("ACTIVITY_KILL_SOLDIER award cann't find item box, account_uuid:",info.account_uuid);
				continue;
			}
			std::vector<ItemTransactionElement> transaction;
			for(auto it = rewards.begin(); it != rewards.end(); ++it){
				const auto item_id = ItemId(it->first);
				const auto count = it->second;
				transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, count,
								ReasonIds::ID_SOLDIER_KILL_RANK,ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER.get(),
								rank,activity_kill_solider_info.available_until);
			}
			item_box->commit_transaction(transaction, false);
		} catch (std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}
	return true;
}

}
