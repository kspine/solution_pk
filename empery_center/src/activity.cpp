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
#include <poseidon/async_job.hpp>
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
#include "cluster_session.hpp"
#include "legion_member.hpp"
#include "singletons/legion_member_map.hpp"
#include "legion.hpp"
#include "singletons/legion_map.hpp"


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
	for(auto it = m_activitys.begin(); it != m_activitys.end(); ++it){
		auto map_activity_id = MapActivityId((it->second).unique_id);
		auto until = (it->second).available_until;
		if(utc_now > until){
			if(map_activity_id == ActivityIds::ID_MAP_ACTIVITY_RESOURCE){
				WorldMap::remove_activity_event(MapEventBlock::MAP_EVENT_ACTIVITY_RESOUCE);
			}else if(map_activity_id == ActivityIds::ID_MAP_ACTIVITY_GOBLIN){
				WorldMap::remove_activity_event(MapEventBlock::MAP_EVENT_ACTIVITY_GOBLIN);
			}
		}
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
	std::vector<MapActivityAccumulateMap::AccumulateInfo> accumuate_vec;
	MapActivityAccumulateMap::get_recent(ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER,activity_kill_solider_info.available_since,accumuate_vec);
	std::sort(accumuate_vec.begin(),accumuate_vec.end(),[](const MapActivityAccumulateMap::AccumulateInfo& left, const MapActivityAccumulateMap::AccumulateInfo& right)
	{
		if(left.accumulate_value > right.accumulate_value){
			return true;
		}
		return false;
	});
	std::uint64_t rank = 1;
	for(auto it = accumuate_vec.begin(); (it != accumuate_vec.end()) && (rank <= 50); ++it,++rank){
		MapActivityRankMap::MapActivityRankInfo info = {};
		info.account_uuid     = (*it).account_uuid;
		info.activity_id      = (*it).activity_id;
		info.settle_date      = activity_kill_solider_info.available_until;
		info.rank             = rank;
		info.accumulate_value = (*it).accumulate_value;
		info.process_date     = now;
		MapActivityRankMap::insert(info);
		//发送奖励
		try{
			Poseidon::enqueue_async_job(
				[=]() mutable {
					PROFILE_ME;
					std::vector<std::pair<std::uint64_t,std::uint64_t>> rewards;
					bool is_award_data = Data::ActivityAward::get_activity_rank_award(ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER.get(),rank,rewards);
					if(!is_award_data){
						LOG_EMPERY_CENTER_WARNING("ACTIVITY_KILL_SOLDIER has no award data, rank:",rank);
						return;
					}
					AccountMap::require_controller_token(info.account_uuid, { });
					const auto item_box = ItemBoxMap::get(info.account_uuid);
					if(!item_box){
						LOG_EMPERY_CENTER_WARNING("ACTIVITY_KILL_SOLDIER award cann't find item box, account_uuid:",info.account_uuid);
						return;
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
				});
		} catch (std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}
	return true;
}

void MapActivity::synchronize_kill_soliders_rank_with_player(const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	std::vector<MapActivityRankMap::MapActivityRankInfo> ret;
	const auto utc_now = Poseidon::get_utc_time();
	MapActivity::MapActivityDetailInfo activity_kill_solider_info =  get_activity_info(MapActivityId(ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER));
	if(activity_kill_solider_info.unique_id != ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER.get()|| utc_now < activity_kill_solider_info.available_until){
		return;
	}
	MapActivityRankMap::get_recent_rank_list(MapActivityId(ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER),activity_kill_solider_info.available_until,ret);
	if(ret.empty()){
		settle_kill_soliders_activity(utc_now);
		MapActivityRankMap::get_recent_rank_list(MapActivityId(ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER),activity_kill_solider_info.available_until,ret);
	}
	if(session){
		try {
			Msg::SC_MapActivityKillSolidersRank msgRankList;
			for(auto it = ret.begin(); it != ret.end(); ++it){
				auto &rank_item = *msgRankList.rank_list.emplace(msgRankList.rank_list.end());
				const auto &account = AccountMap::require(it->account_uuid);
				const auto &castle = WorldMap::require_primary_castle(it->account_uuid);
				const auto legion_member =  LegionMemberMap::get_by_account_uuid(it->account_uuid);
				rank_item.account_uuid      = it->account_uuid.str();
				rank_item.nick              = account->get_nick();
				rank_item.castle_name       = castle->get_name();
				rank_item.leagues           = "";
				if(legion_member){
					const auto legion =  LegionMap::get(legion_member->get_legion_uuid());
					rank_item.leagues =  legion->get_nick();
				}
				rank_item.rank              = it->rank;
				rank_item.accumulate_value  = it->accumulate_value;
			}
			session->send(msgRankList);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}
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
	std::vector<std::pair<Coord, boost::shared_ptr<ClusterSession>>> clusters;
	WorldMap::get_clusters_all(clusters);
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
		account_info.rewarded  = 0;
	}else{
		account_info.accumulate_value += real_delta;
	}
	WorldActivityAccumulateMap::update(account_info);

	if(finish){
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
				WorldActivityBossMap::WorldActivityBossInfo info = WorldActivityBossMap::get(cluster_coord,m_available_since);
				if((*it).finish){
					activity.schedule = info.hp_die;
				}else{
					const auto monster_object = WorldMap::get_map_object(info.boss_uuid);
					if(monster_object){
						activity.schedule = static_cast<std::uint64_t>(monster_object->get_attribute(AttributeIds::ID_HP_TOTAL));
					}else{
						activity.schedule = info.hp_die;
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
			activity.rewarded = account_accumulate_info.rewarded;
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

void WorldActivity::account_accmulate_sort(const Coord cluster_coord,std::vector<std::pair<AccountUuid,std::uint64_t>> &account_accumulate_vec){
	std::vector<WorldActivityAccumulateMap::WorldActivityAccumulateInfo> world_activity_accumulate_vec;
	WorldActivityAccumulateMap::get_recent_activity_accumulate_info(cluster_coord,m_available_since,world_activity_accumulate_vec);
	if(world_activity_accumulate_vec.empty()){
		return;
	}
	boost::container::flat_map<AccountUuid,boost::uint64_t> account_accumulate_map;
	for(auto it = world_activity_accumulate_vec.begin(); it != world_activity_accumulate_vec.end(); ++it){
		auto account_uuid = (*it).account_uuid;
		auto accumulate_value = (*it).accumulate_value;
		auto ita = account_accumulate_map.find(account_uuid);
		if(ita == account_accumulate_map.end()){
			account_accumulate_map.emplace(account_uuid,accumulate_value);
		}else{
			const auto old_accmulate = ita->second;
			ita->second = old_accmulate + accumulate_value;
		}
	}
	for(auto it = account_accumulate_map.begin(); it != account_accumulate_map.end();++it){
		account_accumulate_vec.push_back(std::make_pair(it->first,it->second));
	}
	std::sort(account_accumulate_vec.begin(),account_accumulate_vec.end(),[](const std::pair<AccountUuid,std::uint64_t>& left,const std::pair<AccountUuid,std::uint64_t>& right)
	{
		if(left.second > right.second){
			return true;
		}
		return false;
	});
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
	std::vector<ACCOUNT_ACCUMULATE_PAIR> account_accumulate_vec;
	account_accmulate_sort(cluster_coord,account_accumulate_vec);
	std::uint64_t rank = 1;
	for(auto it = account_accumulate_vec.begin(); (it != account_accumulate_vec.end())&&(rank <= max_rank); ++it,++rank){
		WorldActivityRankMap::WorldActivityRankInfo info = {};
		info.account_uuid     = (*it).first;
		info.cluster_coord    = cluster_coord;
		info.since            = m_available_since;
		info.rank             = rank;
		info.accumulate_value = (*it).second;
		info.process_date     = now;
		info.rewarded         = 0;
		WorldActivityRankMap::insert(std::move(info));
	}

	return true;
}
bool WorldActivity::settle_world_activity_in_activity(Coord cluster_coord,std::uint64_t now,std::vector<WorldActivityRankMap::WorldActivityRankInfo> &ret){
	auto it = m_last_world_accumulates.find(cluster_coord);
	std::vector<ACCOUNT_ACCUMULATE_PAIR> account_accumulate_vec;
	if(it != m_last_world_accumulates.end())
	{
		auto &last_accumulate_pair = it->second;
		auto &last_processed_time  = last_accumulate_pair.first;
		auto &last_accumulate_vec  = last_accumulate_pair.second;
		if(now - last_processed_time < 1*60*1000){
			account_accumulate_vec = last_accumulate_vec;
		}else{
			account_accmulate_sort(cluster_coord,account_accumulate_vec);
			last_processed_time      = now;
			last_accumulate_vec    = account_accumulate_vec;
		}
	}else{
		account_accmulate_sort(cluster_coord,account_accumulate_vec);
		m_last_world_accumulates.emplace(cluster_coord,std::make_pair(now,account_accumulate_vec));
	}
	const auto rank_threshold = Data::Global::as_unsigned(Data::Global::SLOT_WORLD_ACTIVITY_RANK_THRESHOLD);
	std::uint64_t rank = 1;
	for(auto it = account_accumulate_vec.begin(); it != account_accumulate_vec.end()&&(rank <= rank_threshold); ++it,++rank){
		WorldActivityRankMap::WorldActivityRankInfo info = {};
		info.account_uuid     = (*it).first;
		info.cluster_coord    = cluster_coord;
		info.since            = m_available_since;
		info.rank             = rank;
		info.accumulate_value = (*it).second;
		info.process_date     = now;
		ret.push_back(std::move(info));
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
	//活动结束，如果排行榜为空，则进行结算，然后发送
	if(m_available_until < utc_now)
	{
		WorldActivityRankMap::get_recent_rank_list(cluster_coord, m_available_since,rank_threshold,ret);
		if(ret.empty()){
			settle_world_activity(cluster_coord,utc_now);
			WorldActivityRankMap::get_recent_rank_list(cluster_coord, m_available_since,rank_threshold,ret);
		}
		self_info = WorldActivityRankMap::get_account_rank(account_uuid,cluster_coord,m_available_since);
	}
	//活动期间排行榜为实时数据，不写入数据库
	else
	{
		//TODO 改成每分钟结算一次
		settle_world_activity_in_activity(cluster_coord,utc_now,ret);
	}
	std::sort(ret.begin(),ret.end(),
	[](const WorldActivityRankMap::WorldActivityRankInfo &lhs, const WorldActivityRankMap::WorldActivityRankInfo &rhs){
				return lhs.rank < rhs.rank;
			});
	msgRankList.rank = self_info.rank;
	msgRankList.accumulate_value = self_info.accumulate_value;
	msgRankList.rewarded         = self_info.rewarded;
	if(session){
		for(auto it = ret.begin(); it != ret.end(); ++it){
			try {
				auto &rank_item = *msgRankList.rank_list.emplace(msgRankList.rank_list.end());
				const auto &account = AccountMap::require(it->account_uuid);
				const auto &castle = WorldMap::require_primary_castle(it->account_uuid);
				const auto legion_member =  LegionMemberMap::get_by_account_uuid(it->account_uuid);
				rank_item.account_uuid      = it->account_uuid.str();
				rank_item.nick              = account->get_nick();
				rank_item.castle_name       = castle->get_name();
				rank_item.leagues           = "";
				if(legion_member){
					const auto legion =  LegionMap::get(legion_member->get_legion_uuid());
					rank_item.leagues =  legion->get_nick();
				}
				rank_item.rank              = it->rank;
				rank_item.accumulate_value  = it->accumulate_value;
				//实时结算时，需要自己更新数据
				if(account_uuid == it->account_uuid){
					msgRankList.rank = it->rank;
					msgRankList.accumulate_value = it->accumulate_value;
				}
			} catch (std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
		session->send(msgRankList);
	}
}

void WorldActivity::synchronize_world_boss_with_player(const Coord cluster_coord,const boost::shared_ptr<PlayerSession> &session){
	WorldActivityBossMap::WorldActivityBossInfo info = WorldActivityBossMap::get(cluster_coord,m_available_since);
	const auto monster_object = WorldMap::get_map_object(info.boss_uuid);
	Msg::SC_WorldBossPos msg;
	if(monster_object){
		msg.x = monster_object->get_coord().x();
		msg.y = monster_object->get_coord().y();
	}else{
		msg.x = 0;
		msg.y = 0;
	}
	if(session){
		session->send(msg);
	}
}

void WorldActivity::on_activity_expired(){
	std::vector<std::pair<Coord, boost::shared_ptr<ClusterSession>>> clusters;
	WorldMap::get_clusters_all(clusters);
	for(auto it = clusters.begin(); it != clusters.end(); ++it){
		auto cluster_coord = it->first;
		WorldMap::remove_world_activity_event(cluster_coord,MapEventBlock::MAP_EVENT_WORLD_ACTIVITY_MONSTER);
		WorldMap::remove_world_activity_event(cluster_coord,MapEventBlock::MAP_EVENT_COUNTRY_ACTIVIYT_RESOURCE);
		WorldMap::remove_world_activity_boss(cluster_coord,m_available_since);
	}
}

void WorldActivity::reward_activity(Coord cluster_coord,WorldActivityId sub_world_activity_id,AccountUuid account_uuid){
	WorldActivityMap::WorldActivityInfo world_activity_info = WorldActivityMap::get(cluster_coord,sub_world_activity_id,m_available_since);
	if(world_activity_info.activity_id != sub_world_activity_id){
		return ;
	}
	if(!world_activity_info.finish){
		return ;
	}
	WorldActivityAccumulateMap::WorldActivityAccumulateInfo account_accumulate_info = WorldActivityAccumulateMap::get(account_uuid,cluster_coord, sub_world_activity_id,m_available_since);
	if(account_accumulate_info.rewarded){
		return;
	}
	if(account_accumulate_info.activity_id != sub_world_activity_id){
		//没有参与活动不会发送奖励
		return;
	}
	boost::shared_ptr<const Data::WorldActivity> world_activity_data = Data::WorldActivity::get(sub_world_activity_id.get());
	if(!world_activity_data){
		LOG_EMPERY_CENTER_WARNING("no world activity data, world_activity_id = ",sub_world_activity_id.get());
		return;
	}
	try{
		const auto item_box = ItemBoxMap::get(account_uuid);
		if(!item_box){
			LOG_EMPERY_CENTER_WARNING("world activity award cann't find item box, account_uuid:",account_uuid, ", cluster_coord = ", cluster_coord, ", world_activity_id = ", sub_world_activity_id);
			return;
		}
		std::vector<ItemTransactionElement> transaction;
		for(auto rit = world_activity_data->rewards.begin(); rit != world_activity_data->rewards.end(); ++rit){
			const auto &collection_name = rit->first;
			auto repeat_count = rit->second;
			for(std::size_t i = 0; i < repeat_count; ++i){
				const auto reward_data = Data::MapObjectTypeMonsterReward::random_by_collection_name(collection_name);
				if(!reward_data){
					LOG_EMPERY_CENTER_WARNING("Error getting random reward: account_uuid:",account_uuid, ", cluster_coord = ", cluster_coord, ", world_activity_id = ", sub_world_activity_id,
						", collection_name = ", collection_name);
					continue;
				}
				for(auto it = reward_data->reward_items.begin(); it != reward_data->reward_items.end(); ++it){
					const auto item_id = it->first;
					const auto count = it->second;
					transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, count,
						ReasonIds::ID_WORLD_ACTIVITY, sub_world_activity_id.get(),
						cluster_coord.x(), cluster_coord.y());
				}
			}
		}
		item_box->commit_transaction(transaction, false);
	}  catch (std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}
	account_accumulate_info.rewarded  = true;
	WorldActivityAccumulateMap::update(account_accumulate_info);
}

void WorldActivity::reward_rank(Coord cluster_coord,AccountUuid account_uuid){
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < m_available_until){
		return;
	}
	WorldActivityRankMap::WorldActivityRankInfo self_info = {};
	self_info = WorldActivityRankMap::get_account_rank(account_uuid,cluster_coord,m_available_since);
	if(self_info.rank == 0){
		return;
	}
	if(self_info.rewarded){
		return;
	}
	//发送奖励
	try{
		std::vector<std::pair<std::uint64_t,std::uint64_t>> rewards;
		bool is_award_data = Data::ActivityAward::get_activity_rank_award(ActivityIds::ID_WORLD_ACTIVITY.get(),self_info.rank,rewards);
		if(!is_award_data){
			LOG_EMPERY_CENTER_WARNING("COUNTRY ACTIVITY has no award data, rank:",self_info.rank);
			return;
		}
		const auto item_box = ItemBoxMap::get(account_uuid);
		if(!item_box){
			LOG_EMPERY_CENTER_WARNING("COUNTRY ACTIVITY award cann't find item box, account_uuid:",account_uuid);
			return;
		}
		std::vector<ItemTransactionElement> transaction;
		for(auto it = rewards.begin(); it != rewards.end(); ++it){
			const auto item_id = ItemId(it->first);
			const auto count = it->second;
			transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, count,
							ReasonIds::ID_WORLD_ACTIVITY_RANK,ActivityIds::ID_WORLD_ACTIVITY.get(),
							self_info.rank,m_available_since);
		}
		item_box->commit_transaction(transaction, false);
	} catch (std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}
}

}
