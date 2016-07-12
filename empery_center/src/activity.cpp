#include "precompiled.hpp"
#include "activity.hpp"
#include "data/activity.hpp"
#include "data/map_object_type.hpp"
#include "data/global.hpp"
#include "activity_ids.hpp"
#include "msg/sc_activity.hpp"
#include "singletons/world_map.hpp"
#include "singletons/map_activity_accumulate_map.hpp"
#include "singletons/account_map.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "map_event_block.hpp"
#include "reason_ids.hpp"
#include "singletons/item_box_map.hpp"
#include "item_box.hpp"
#include "mysql/activity.hpp"
#include "transaction_element.hpp"
#include "castle.hpp"
#include "account.hpp"
#include "map_object_type_ids.hpp"
#include "attribute_ids.hpp"


namespace EmperyCenter {

Activity::Activity(std::uint64_t unique_id_,std::uint64_t available_since_,std::uint64_t available_until_):
	m_unique_id(unique_id_),m_available_since(available_since_),m_available_until(available_until_)
	{
	}

Activity::~Activity(){
}

void Activity::pump_status(){
}

MapActivity::MapActivity(std::uint64_t unique_id_,std::uint64_t available_since_,std::uint64_t available_until_)
	: Activity(unique_id_,available_since_,available_until_),m_current_activity(MapActivityId(0)),m_next_activity_time(0)
{
	PROFILE_ME;

	std::vector<boost::shared_ptr<const Data::MapActivity>> map_activity_data;
	Data::MapActivity::get_all(map_activity_data);
	std::uint64_t  last_util_time = m_available_since;
	std::uint64_t  new_until_time = m_available_since;
	for(auto it = map_activity_data.begin(); it != map_activity_data.end(); it++){
		new_until_time = saturated_add(last_util_time, static_cast<std::uint64_t>((*it)->continued_time * 60000));
		if(new_until_time > m_available_until){
			 LOG_EMPERY_CENTER_DEBUG("activity time over end time ");                               
			 DEBUG_THROW(Exception, sslit("activity time over end time "));
		}
		MapActivityDetailInfo info;
		info.unique_id = (*it)->unique_id;
		info.available_since = last_util_time;
		info.available_until = new_until_time;
		last_util_time = new_until_time;
		m_activitys.emplace(MapActivityId(info.unique_id),std::move(info));
	}
}

MapActivity::~MapActivity(){
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

WorldActivity::WorldActivity(std::uint64_t unique_id_,std::uint64_t available_since_,std::uint64_t available_until_)
	: Activity(unique_id_,available_since_,available_until_),m_expired_remove(false){
	}

WorldActivity::~WorldActivity(){
}

void WorldActivity::pump_status(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();
	if((utc_now > m_available_until)&& !m_expired_remove){
		on_activity_expired();
		m_expired_remove = true;
	}
	if(utc_now < m_available_since || utc_now > m_available_until){
		return;
	}
	boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> clusters;
	WorldMap::get_all_clusters(clusters);
	for(auto it = clusters.begin(); it != clusters.end(); ++it){
		WorldActivityMap::WorldActivityInfo info = WorldActivityMap::get(it->first,ActivityIds::ID_WORLD_ACTIVITY_MONSTER,m_available_since);
		if(info.activity_id != ActivityIds::ID_WORLD_ACTIVITY_MONSTER){
			info.activity_id = ActivityIds::ID_WORLD_ACTIVITY_MONSTER;
			info.cluster_coord = it->first;
			info.since = m_available_since;
			info.sub_since = utc_now;
			info.sub_until = 0;
			info.accumulate_value = 0;
			info.finish = false;
			WorldActivityMap::insert(std::move(info));
			on_activity_change(WorldActivityId(0),ActivityIds::ID_WORLD_ACTIVITY_MONSTER,info.cluster_coord);
		}
	}
}

void WorldActivity::on_activity_change(WorldActivityId old_activity, WorldActivityId new_activity,Coord cluster_coord){
	if(old_activity == ActivityIds::ID_WORLD_ACTIVITY_MONSTER){
		WorldMap::remove_world_activity_event(cluster_coord,MapEventBlock::MAP_EVENT_WORLD_ACTIVITY_MONSTER);
	}
	if(old_activity == ActivityIds::ID_WORLD_ACTIVITY_RESOURCE){
		WorldMap::remove_world_activity_event(cluster_coord,MapEventBlock::MAP_EVENT_COUNTRY_ACTIVIYT_RESOURCE);
	}
	if(new_activity == ActivityIds::ID_WORLD_ACTIVITY_MONSTER){
		WorldMap::refresh_world_activity_event(cluster_coord,MapEventBlock::MAP_EVENT_WORLD_ACTIVITY_MONSTER);
	}
	if(new_activity == ActivityIds::ID_WORLD_ACTIVITY_RESOURCE){
		WorldMap::refresh_world_activity_event(cluster_coord,MapEventBlock::MAP_EVENT_COUNTRY_ACTIVIYT_RESOURCE);
	}
	if(new_activity == ActivityIds::ID_WORLD_ACTIVITY_BOSS){
		WorldMap::refresh_world_activity_boss(cluster_coord,m_available_since);
	}
}

bool WorldActivity::is_on(){
	PROFILE_ME;
	
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < m_available_since || utc_now > m_available_until){
		return false;
	}
	return true;
}

bool WorldActivity::is_world_activity_on(Coord cluster_coord,WorldActivityId world_activity_id){
	PROFILE_ME;
	
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < m_available_since || utc_now > m_available_until){
		return false;
	}
	WorldActivityMap::WorldActivityInfo info = WorldActivityMap::get(cluster_coord,world_activity_id,m_available_since);
	if(info.activity_id != world_activity_id){
		return false;
	}
	if(info.finish){
		return false;
	}
	return true;
}

void WorldActivity::update_world_activity_schedule(Coord cluster_coord,WorldActivityId world_activity_id,AccountUuid account_uuid,std::uint64_t delta,bool boss_die){
	PROFILE_ME;
	
	if(!is_world_activity_on(cluster_coord,world_activity_id)){
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();
	WorldActivityMap::WorldActivityInfo info = WorldActivityMap::get(cluster_coord,world_activity_id,m_available_since);
	if(info.activity_id != world_activity_id || info.finish){
		return;
	}
	boost::shared_ptr<const Data::WorldActivity> world_activity_data = Data::WorldActivity::get(world_activity_id.get());
	if(!world_activity_data){
		LOG_EMPERY_CENTER_DEBUG("no world activity data, world_activity_id = ",world_activity_id);
		return;
	}
	auto new_accumulate = info.accumulate_value + delta;
	auto real_delta = delta;
	bool finish = false;
	if(world_activity_id != ActivityIds::ID_WORLD_ACTIVITY_BOSS){
		for(auto it = world_activity_data->objective.begin(); it != world_activity_data->objective.end(); ++it){
			const auto target = it->second;
			if(target <= new_accumulate){
				info.accumulate_value = target;
				info.finish = true;
				info.sub_until = utc_now;
				finish = true;
				real_delta -= (new_accumulate - target );
			}else{
				info.accumulate_value = new_accumulate;
			}
		break;
		}
	} else {
		info.accumulate_value = new_accumulate;
		if(boss_die){
			info.finish = true;
			info.sub_until = utc_now;
			finish = true;
			WorldActivityBossMap::WorldActivityBossInfo boss_info = WorldActivityBossMap::get(cluster_coord,m_available_since);
			if(boss_info.cluster_coord == cluster_coord){
				boss_info.delete_date = utc_now;
				WorldActivityBossMap::update(boss_info);
			}
		}
	}
	
	//更新活动进度
	WorldActivityMap::update(info,false);

	//更新个人进度
	WorldActivityAccumulateMap::WorldActivityAccumulateInfo account_info =  WorldActivityAccumulateMap::get(account_uuid,cluster_coord,world_activity_id,m_available_since);
	if(account_info.activity_id != world_activity_id){
		account_info.account_uuid = account_uuid;
		account_info.activity_id = world_activity_id;
		account_info.cluster_coord = cluster_coord;
		account_info.since = m_available_since;
		account_info.accumulate_value = real_delta;
	}else{
		account_info.accumulate_value += real_delta;
	}
	WorldActivityAccumulateMap::update(account_info);

	if(finish){
		//给参与的玩家发送阶段奖励
		std::vector<AccountUuid> ret;
		WorldActivityAccumulateMap::get_recent_world_activity_account(cluster_coord,world_activity_id,m_available_since,ret);
		for(auto it = ret.begin(); it != ret.end(); ++it){
			try{
				const auto item_box = ItemBoxMap::get(*it);
				if(!item_box){
					LOG_EMPERY_CENTER_WARNING("world activity award cann't find item box, account_uuid:",account_uuid, ", cluster_coord = ", cluster_coord, ", world_activity_id = ", world_activity_id);
					continue;
				}
				std::vector<ItemTransactionElement> transaction;
				for(auto rit = world_activity_data->rewards.begin(); rit != world_activity_data->rewards.end(); ++rit){
					const auto &collection_name = rit->first;
					auto repeat_count = rit->second;
					for(std::size_t i = 0; i < repeat_count; ++i){
						const auto reward_data = Data::MapObjectTypeMonsterReward::random_by_collection_name(collection_name);
						if(!reward_data){
							LOG_EMPERY_CENTER_WARNING("Error getting random reward: account_uuid:",account_uuid, ", cluster_coord = ", cluster_coord, ", world_activity_id = ", world_activity_id,
								", collection_name = ", collection_name);
							continue;
						}
						for(auto it = reward_data->reward_items.begin(); it != reward_data->reward_items.end(); ++it){
							const auto item_id = it->first;
							const auto count = it->second;
							transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, count,
								ReasonIds::ID_WORLD_ACTIVITY, world_activity_id.get(),
								cluster_coord.x(), cluster_coord.y());
						}
					}
				}
			}  catch (std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
		
		//如果有下一阶段，则进行下一阶段
		std::vector<boost::shared_ptr<const Data::WorldActivity>> world_activity_vec;
		Data::WorldActivity::get_all(world_activity_vec);
		for(auto it = world_activity_vec.begin(); it != world_activity_vec.end(); ++it){
			if((*it)->pre_unique_id == world_activity_id.get()){
				WorldActivityMap::WorldActivityInfo info;
				info.activity_id = WorldActivityId((*it)->unique_id);
				info.cluster_coord = cluster_coord;
				info.since = m_available_since;
				info.sub_since = utc_now;
				info.sub_until = 0;
				info.accumulate_value = 0;
				info.finish = false;
				WorldActivityMap::insert(info);
				on_activity_change(world_activity_id,WorldActivityId(info.activity_id),cluster_coord);
				break;
			}
		}
	}
}

void WorldActivity::synchronize_with_player(const Coord cluster_coord,AccountUuid account_uuid,const boost::shared_ptr<PlayerSession> &session) const{
	PROFILE_ME;
	
	std::vector<WorldActivityMap::WorldActivityInfo> ret;
	WorldActivityMap::get_recent_world_activity(cluster_coord,m_available_since,ret);
	try{
		Msg::SC_WorldActivityInfo msg;
		msg.since = m_available_since;
		msg.until = m_available_until;
		std::uint64_t curr_activity_id = 0;
		for(auto it = ret.begin(); it != ret.end(); ++it){
			auto &activity = *msg.activitys.emplace(msg.activitys.end());
			activity.unique_id    = (*it).activity_id.get();
			activity.sub_since = (*it).sub_since;
			activity.sub_until   = (*it).sub_until;
			activity.finish       = (*it).finish;
			
			if((*it).activity_id == ActivityIds::ID_WORLD_ACTIVITY_BOSS){
				const auto monster_data = Data::MapObjectTypeMonster::require(MapObjectTypeIds::ID_WORLD_ACTIVITY_BOSS);
				const auto hp_total = checked_mul(monster_data->max_soldier_count, monster_data->hp_per_soldier);
				activity.objective = hp_total;
				if((*it).finish){
					activity.schedule = 0;
				}else{
					WorldActivityBossMap::WorldActivityBossInfo info = WorldActivityBossMap::get(cluster_coord,m_available_since);
					if(info.cluster_coord != cluster_coord){
						activity.schedule = 0;
					}else{
						const auto monster_object = WorldMap::get_map_object(info.boss_uuid);
						if(monster_object){
							activity.schedule = static_cast<std::uint64_t>(monster_object->get_attribute(AttributeIds::ID_HP_TOTAL));
						}else{
							activity.schedule = 0;
						}
					}
				}
			}
			else{
				//目标值
				boost::shared_ptr<const Data::WorldActivity> world_activity_data = Data::WorldActivity::get((*it).activity_id.get());
				if(!world_activity_data){
					LOG_EMPERY_CENTER_DEBUG("no world activity data, world_activity_id = ",(*it).activity_id.get());
					continue;
				}
				std::uint64_t objective = 0;
				for(auto it = world_activity_data->objective.begin(); it != world_activity_data->objective.end(); ++it){
					objective = it->second;
					break;
				}
				activity.objective = objective;
				activity.schedule  = (*it).accumulate_value;
			}
			
			//个人贡献
			WorldActivityAccumulateMap::WorldActivityAccumulateInfo account_accumulate_info =  WorldActivityAccumulateMap::get(account_uuid,cluster_coord,(*it).activity_id,m_available_since);
			if(account_accumulate_info.activity_id != (*it).activity_id){
				activity.contribute   =  0;
			}else{
				activity.contribute   =  account_accumulate_info.accumulate_value;
			}
			
			if(!((*it).finish)){
				curr_activity_id = (*it).activity_id.get();
			}
			
		}
		msg.curr_activity_id = curr_activity_id;
		session->send(msg);
	} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}
}

bool WorldActivity::settle_world_activity(Coord cluster_coord,std::uint64_t now){
	if(now < m_available_until){
		return false;
	}
	std::vector<WorldActivityRankMap::WorldActivityRankInfo> ret;
	WorldActivityRankMap::get_recent_rank_list(cluster_coord,m_available_since,1,ret);
	if(!ret.empty()){
		return false;
	}
	
	std::uint64_t max_rank = Data::ActivityAward::get_max_activity_award_rank(ActivityIds::ID_WORLD_ACTIVITY.get());
	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapCountryStatics>>>();
	{
		std::ostringstream oss;
		char str[256];
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<std::uint64_t>(m_available_since), false);
		oss <<"SELECT account_uuid,sum(accumulate_value) as accumulate_value FROM `Center_MapWorldActivityAccumulate` WHERE `since` = " << Poseidon::MySql::StringEscaper(str) << " and `cluster_x` = " << cluster_coord.x()  << " and `cluster_y` = " << cluster_coord.y() << " GROUP BY account_uuid order by accumulate_value desc limit " << max_rank;
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Center_MapCountryStatics>();
				obj->fetch(conn);
				sink->emplace_back(std::move(obj));
			}, "Center_MapWorldActivityRank", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		return false;
	}
	std::uint64_t rank = 1;
	for(auto it = sink->begin(); it != sink->end(); ++it,++rank){
		WorldActivityRankMap::WorldActivityRankInfo info = {};
		info.account_uuid     = AccountUuid((*it)->get_account_uuid());
		info.cluster_coord    = cluster_coord;
		info.since            = m_available_since;
		info.rank             = rank;
		info.accumulate_value = (*it)->get_accumulate_value();
		info.process_date     = now;
		WorldActivityRankMap::insert(std::move(info));
		//发送奖励
		try{
			std::vector<std::pair<std::uint64_t,std::uint64_t>> rewards;
			bool is_award_data = Data::ActivityAward::get_activity_rank_award(ActivityIds::ID_WORLD_ACTIVITY.get(),rank,rewards);
			if(!is_award_data){
				LOG_EMPERY_CENTER_WARNING("COUNTRY ACTIVITY has no award data, rank:",rank);
				continue;
			}
			const auto item_box = ItemBoxMap::get(info.account_uuid);
			if(!item_box){
				LOG_EMPERY_CENTER_WARNING("COUNTRY ACTIVITY award cann't find item box, account_uuid:",info.account_uuid);
				continue;
			}
			std::vector<ItemTransactionElement> transaction;
			for(auto it = rewards.begin(); it != rewards.end(); ++it){
				const auto item_id = ItemId(it->first);
				const auto count = it->second;
				transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, count,
								ReasonIds::ID_WORLD_ACTIVITY_RANK,ActivityIds::ID_WORLD_ACTIVITY.get(),
								rank,m_available_since);
			}
			item_box->commit_transaction(transaction, false);
		} catch (std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}
	return true;
}

void WorldActivity::synchronize_world_rank_with_player(const Coord cluster_coord,AccountUuid account_uuid,const boost::shared_ptr<PlayerSession> &session){
	const auto utc_now = Poseidon::get_utc_time();
	const auto rank_threshold = Data::Global::as_unsigned(Data::Global::SLOT_WORLD_ACTIVITY_RANK_THRESHOLD);
	std::vector<WorldActivityRankMap::WorldActivityRankInfo> ret;
	WorldActivityRankMap::WorldActivityRankInfo self_info = {};
	Msg::SC_WorldActivityRank msgRankList;
	msgRankList.since = m_available_since;
	//当前时间小于活动时间发送上次排行榜
	if(utc_now < m_available_since){
		//获取最近一次的时间
		std::uint64_t pro_time = WorldActivityRankMap::get_pro_rank_time(cluster_coord,m_available_since);
		if(pro_time != 0){
			msgRankList.since = pro_time;
			WorldActivityRankMap::get_recent_rank_list(cluster_coord, pro_time,rank_threshold,ret);
			self_info = WorldActivityRankMap::get_account_rank(account_uuid,cluster_coord,pro_time);
		}
	}
	//活动结束，如果排行榜为空，则进行结算，然后发送
	else if(m_available_until < utc_now)
	{
		WorldActivityRankMap::get_recent_rank_list(cluster_coord, m_available_since,rank_threshold,ret);
		if(ret.empty()){
			settle_world_activity(cluster_coord,utc_now);
			WorldActivityRankMap::get_recent_rank_list(cluster_coord, m_available_since,rank_threshold,ret);
		}
		self_info = WorldActivityRankMap::get_account_rank(account_uuid,cluster_coord,m_available_since);
	}
	//活动期间排行榜为空
	else
	{
	}
	msgRankList.rank = self_info.rank;
	msgRankList.accumulate_value = self_info.accumulate_value;
	if(session){
		for(auto it = ret.begin(); it != ret.end(); ++it){
			try {
				auto &rank_item = *msgRankList.rank_list.emplace(msgRankList.rank_list.end());
				const auto &account = AccountMap::require(it->account_uuid);
				const auto &castle = WorldMap::require_primary_castle(it->account_uuid);
				rank_item.account_uuid      = it->account_uuid.str();
				rank_item.nick              = account->get_nick();
				rank_item.castle_name       = castle->get_name();
				rank_item.leagues           = "";
				rank_item.rank              = it->rank;
				rank_item.accumulate_value  = it->accumulate_value;
			} catch (std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
		session->send(msgRankList);
	}
	
}

void WorldActivity::on_activity_expired(){
	boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> clusters;
	WorldMap::get_all_clusters(clusters);
	for(auto it = clusters.begin(); it != clusters.end(); ++it){
		auto cluster_coord = it->first;
		WorldMap::remove_world_activity_event(cluster_coord,MapEventBlock::MAP_EVENT_WORLD_ACTIVITY_MONSTER);
		WorldMap::remove_world_activity_event(cluster_coord,MapEventBlock::MAP_EVENT_COUNTRY_ACTIVIYT_RESOURCE);
		WorldMap::remove_world_activity_boss(cluster_coord,m_available_since);
	}
}

}
