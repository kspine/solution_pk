#include "../precompiled.hpp"
#include "map_activity_accumulate_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include "../mysql/activity.hpp"


namespace EmperyCenter {

namespace {
	struct MapActivityAccumulateElement {
		boost::shared_ptr<MySql::Center_MapActivityAccumulate> obj;
		std::pair<AccountUuid, std::pair<MapActivityId, std::uint64_t>> account_activity_recent;
		std::pair<MapActivityId,std::uint64_t>  activity_recent;

		MapActivityAccumulateElement(boost::shared_ptr<MySql::Center_MapActivityAccumulate> obj_)
			:obj(std::move(obj_)),account_activity_recent(std::make_pair(AccountUuid(obj->get_account_uuid()),std::make_pair(MapActivityId(obj->get_map_activity_id()),obj->get_avaliable_since())))
			,activity_recent(std::make_pair(MapActivityId(obj->get_map_activity_id()),obj->get_avaliable_since()))
		{
		}
	};

	MULTI_INDEX_MAP(MapActivityAccumulateContainer, MapActivityAccumulateElement,
		UNIQUE_MEMBER_INDEX(account_activity_recent)
		MULTI_MEMBER_INDEX(activity_recent)
	)

	struct MapActivityRankElement {
		boost::shared_ptr<MySql::Center_MapActivityRank> obj;
		std::pair<MapActivityId, std::uint64_t> activity_recent;
		std::pair<AccountUuid, std::pair<MapActivityId, std::uint64_t> > account_activity_recent;
		MapActivityRankElement(boost::shared_ptr<MySql::Center_MapActivityRank> obj_)
			:obj(std::move(obj_)),activity_recent(MapActivityId(obj->get_map_activity_id()),obj->get_settle_date())
			,account_activity_recent(AccountUuid(obj->get_account_uuid()),activity_recent)
		{
		}
	};

	MULTI_INDEX_MAP(MapActivityRankContainer, MapActivityRankElement,
		UNIQUE_MEMBER_INDEX(account_activity_recent)
		MULTI_MEMBER_INDEX(activity_recent)
	)

	struct WorldActivityElement {
		boost::shared_ptr<MySql::Center_MapWorldActivity>                        obj;
		std::pair<std::uint64_t,Coord>                                             recent_cluster_coord;
		std::pair<WorldActivityId,std::pair<std::uint64_t,Coord>>                recent_cluster_activity;
		WorldActivityElement(boost::shared_ptr<MySql::Center_MapWorldActivity> obj_)
			:obj(std::move(obj_)),recent_cluster_coord(std::make_pair(obj->get_since(),Coord(obj->get_cluster_x(),obj->get_cluster_y())))
			, recent_cluster_activity(std::make_pair(WorldActivityId(obj->get_activity_id()),recent_cluster_coord))
		{
		}
	};
	
	MULTI_INDEX_MAP(WorldActivityContainer, WorldActivityElement,
		UNIQUE_MEMBER_INDEX(recent_cluster_activity)
		MULTI_MEMBER_INDEX(recent_cluster_coord)
	)
	
	struct WorldActivityAccumulateElement {
		boost::shared_ptr<MySql::Center_MapWorldActivityAccumulate>                       obj;
		std::pair<std::uint64_t,Coord>                                                    recent_cluster_coord;
		std::pair<WorldActivityId,std::pair<std::uint64_t,Coord>>                         recent_cluster_coord_activity;
		std::pair<std::pair<AccountUuid,WorldActivityId>,std::pair<std::uint64_t,Coord>>  account_recent_cluster_activity;
		WorldActivityAccumulateElement(boost::shared_ptr<MySql::Center_MapWorldActivityAccumulate> obj_)
			:obj(std::move(obj_)),recent_cluster_coord(std::make_pair(obj->get_since(),Coord(obj->get_cluster_x(),obj->get_cluster_y())))
			,recent_cluster_coord_activity(std::make_pair(WorldActivityId(obj->get_activity_id()),recent_cluster_coord))
			,account_recent_cluster_activity(std::make_pair(std::make_pair(AccountUuid(obj->get_account_uuid()),WorldActivityId(obj->get_activity_id())),recent_cluster_coord))
		{
		}
   };
	
	MULTI_INDEX_MAP(WorldActivityAccumulateContainer, WorldActivityAccumulateElement,
		UNIQUE_MEMBER_INDEX(account_recent_cluster_activity)
		MULTI_MEMBER_INDEX(recent_cluster_coord)
		MULTI_MEMBER_INDEX(recent_cluster_coord_activity)
	)
	
	struct WorldActivityRankElement {
		boost::shared_ptr<MySql::Center_MapWorldActivityRank>                    obj;
		std::pair<Coord,std::uint64_t>                                             cluster_recent;
		std::pair<AccountUuid,std::pair<Coord,std::uint64_t>>                      account_cluster_recent;
		WorldActivityRankElement(boost::shared_ptr<MySql::Center_MapWorldActivityRank> obj_)
			:obj(std::move(obj_)),cluster_recent(Coord(obj->get_cluster_x(),obj->get_cluster_y()),obj->get_since())
			,account_cluster_recent(AccountUuid(obj->get_account_uuid()),cluster_recent)
		{
		}
	};
	
	MULTI_INDEX_MAP(WorldActivityRankContainer, WorldActivityRankElement,
		UNIQUE_MEMBER_INDEX(account_cluster_recent)
		MULTI_MEMBER_INDEX(cluster_recent)
	)
	
	struct WorldActivityBossElement {
		boost::shared_ptr<MySql::Center_MapWorldActivityBoss>                    obj;
		std::pair<Coord,std::uint64_t>                                             cluster_recent;
		WorldActivityBossElement(boost::shared_ptr<MySql::Center_MapWorldActivityBoss> obj_)
			:obj(std::move(obj_)),cluster_recent(Coord(obj->get_cluster_x(),obj->get_cluster_y()),obj->get_since())
		{
		}
	};
	
	MULTI_INDEX_MAP(WorldActivityBossContainer, WorldActivityBossElement,
		UNIQUE_MEMBER_INDEX(cluster_recent)
	)

	boost::weak_ptr<MapActivityAccumulateContainer>         g_map_activity_accumulate_map;
	boost::weak_ptr<MapActivityRankContainer>               g_map_activity_rank_map;
	boost::weak_ptr<WorldActivityContainer>               g_world_activity_map;
	boost::weak_ptr<WorldActivityAccumulateContainer>     g_world_activity_accumulate_map;
	boost::weak_ptr<WorldActivityRankContainer>           g_world_activity_rank_map;
	boost::weak_ptr<WorldActivityBossContainer>           g_world_activity_boss_map;
	
	void fill_map_activity_accumulate_info(MapActivityAccumulateMap::AccumulateInfo &info, const MapActivityAccumulateElement &elem){
		PROFILE_ME;

		info.account_uuid     = AccountUuid(elem.obj->get_account_uuid());
		info.activity_id      = MapActivityId(elem.obj->get_map_activity_id());
		info.avaliable_since  = elem.obj->get_avaliable_since();
		info.avaliable_util   = elem.obj->get_avaliable_util();
		info.accumulate_value = elem.obj->get_accumulate_value();
	}

	void fill_map_activity_rank_info(MapActivityRankMap::MapActivityRankInfo &info, const MapActivityRankElement &elem){
		PROFILE_ME;

		info.account_uuid     = AccountUuid(elem.obj->get_account_uuid());
		info.activity_id      = MapActivityId(elem.obj->get_map_activity_id());
		info.settle_date      = elem.obj->get_settle_date();
		info.rank             = elem.obj->get_rank();
		info.process_date     = elem.obj->get_process_date();
		info.accumulate_value = elem.obj->get_accumulate_value();
	}
	
	void fill_world_activity_info(WorldActivityMap::WorldActivityInfo &info, const WorldActivityElement &elem){
		PROFILE_ME;

		info.activity_id      = WorldActivityId(elem.obj->get_activity_id());
		info.cluster_coord    = Coord(elem.obj->get_cluster_x(), elem.obj->get_cluster_y());
		info.since            = elem.obj->get_since();
		info.sub_since        = elem.obj->get_sub_since();
		info.sub_until        = elem.obj->get_sub_until();
		info.accumulate_value = elem.obj->get_accumulate_value();
		info.finish           = elem.obj->get_finish();
	}
	
	void fill_world_activity_accumluate_info(WorldActivityAccumulateMap::WorldActivityAccumulateInfo &info, const WorldActivityAccumulateElement &elem){
		PROFILE_ME;
		info.account_uuid     = AccountUuid(elem.obj->get_account_uuid()); 
		info.activity_id      = WorldActivityId(elem.obj->get_activity_id());
		info.cluster_coord    = Coord(elem.obj->get_cluster_x(), elem.obj->get_cluster_y());
		info.since            = elem.obj->get_since();
		info.accumulate_value = elem.obj->get_accumulate_value();
	}
	
	void fill_world_activity_rank_info(WorldActivityRankMap::WorldActivityRankInfo &info, const WorldActivityRankElement &elem){
		PROFILE_ME;

		info.account_uuid     = AccountUuid(elem.obj->get_account_uuid());
		info.cluster_coord    = Coord(elem.obj->get_cluster_x(), elem.obj->get_cluster_y());
		info.since            = elem.obj->get_since();
		info.rank             = elem.obj->get_rank();
		info.accumulate_value = elem.obj->get_accumulate_value();
		info.process_date     = elem.obj->get_process_date();
	}
	
	void fill_world_activity_boss_info(WorldActivityBossMap::WorldActivityBossInfo &info, const WorldActivityBossElement &elem){
		PROFILE_ME;

		info.cluster_coord    = Coord(elem.obj->get_cluster_x(), elem.obj->get_cluster_y());
		info.since            = elem.obj->get_since();
		info.boss_uuid        = MapObjectUuid(elem.obj->get_boss_uuid());
		info.create_date      = elem.obj->get_create_date();
		info.delete_date      = elem.obj->get_delete_date();
		info.hp_total         = elem.obj->get_hp_total();
		info.hp_die           = elem.obj->get_hp_die();
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto map_activity_accumulate_map = boost::make_shared<MapActivityAccumulateContainer>();
		LOG_EMPERY_CENTER_INFO("Loading map activity accmulate ...");
		const auto conn = Poseidon::MySqlDaemon::create_connection();
		conn->execute_sql("SELECT * FROM `Center_MapActivityAccumulate` ");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapActivityAccumulate>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			map_activity_accumulate_map->insert(MapActivityAccumulateElement(std::move(obj)));
		}
		g_map_activity_accumulate_map = map_activity_accumulate_map;
		handles.push(map_activity_accumulate_map);

		const auto map_activity_rank_map = boost::make_shared<MapActivityRankContainer>();
		LOG_EMPERY_CENTER_INFO("Loading map activity rank ...");
		conn->execute_sql("SELECT * FROM `Center_MapActivityRank` ");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapActivityRank>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			map_activity_rank_map->insert(MapActivityRankElement(std::move(obj)));
		}
		g_map_activity_rank_map = map_activity_rank_map;
		handles.push(map_activity_rank_map);
		
		
		const auto world_activity_map = boost::make_shared<WorldActivityContainer>();
		LOG_EMPERY_CENTER_INFO("Loading map world activity ...");
		conn->execute_sql("SELECT * FROM `Center_MapWorldActivity` ");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapWorldActivity>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			world_activity_map->insert(WorldActivityElement(std::move(obj)));
		}
		g_world_activity_map = world_activity_map;
		handles.push(world_activity_map);
		
		
		const auto world_activity_accumulate_map = boost::make_shared<WorldActivityAccumulateContainer>();
		LOG_EMPERY_CENTER_INFO("Loading map world activity accumulate...");
		conn->execute_sql("SELECT * FROM `Center_MapWorldActivityAccumulate` ");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapWorldActivityAccumulate>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			world_activity_accumulate_map->insert(WorldActivityAccumulateElement(std::move(obj)));
		}
		g_world_activity_accumulate_map = world_activity_accumulate_map;
		handles.push(world_activity_accumulate_map);
		
		
		const auto world_activity_rank_map = boost::make_shared<WorldActivityRankContainer>();
		LOG_EMPERY_CENTER_INFO("Loading  world activity rank...");
		conn->execute_sql("SELECT * FROM `Center_MapWorldActivityRank` ");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapWorldActivityRank>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			world_activity_rank_map->insert(WorldActivityRankElement(std::move(obj)));
		}
		g_world_activity_rank_map = world_activity_rank_map;
		handles.push(world_activity_rank_map);
		
		
		const auto world_activity_boss_map = boost::make_shared<WorldActivityBossContainer>();
		LOG_EMPERY_CENTER_INFO("Loading  world activity boss info...");
		conn->execute_sql("SELECT * FROM `Center_MapWorldActivityBoss` ");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapWorldActivityBoss>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			world_activity_boss_map->insert(WorldActivityBossElement(std::move(obj)));
		}
		g_world_activity_boss_map = world_activity_boss_map;
		handles.push(world_activity_boss_map);
	}
}

MapActivityAccumulateMap::AccumulateInfo MapActivityAccumulateMap::get(AccountUuid account_uuid, MapActivityId activity_id,std::uint64_t since){
	PROFILE_ME;

	AccumulateInfo info = {};
	const auto map_activity_accumulate_map = g_map_activity_accumulate_map.lock();
	if(!map_activity_accumulate_map){
		LOG_EMPERY_CENTER_WARNING("map activity accumulate map is not loaded.");
		return info;
	}
	const auto it = map_activity_accumulate_map->find<0>(std::make_pair(account_uuid, std::make_pair(activity_id,since)));
	if(it == map_activity_accumulate_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("map activity accumulate not found: account_uuid = ", account_uuid, ", activity_id = ", activity_id.get());
		return info;
	}
	fill_map_activity_accumulate_info(info, *it);
	return info;
}

void MapActivityAccumulateMap::get_recent(MapActivityId activity_id,std::uint64_t since,std::vector<AccumulateInfo> &ret){
	PROFILE_ME;

	const auto map_activity_accumulate_map = g_map_activity_accumulate_map.lock();
	if(!map_activity_accumulate_map){
		LOG_EMPERY_CENTER_WARNING("map activity accumulate map is not loaded.");
		return;
	}
	const auto range = map_activity_accumulate_map->equal_range<1>(std::make_pair(activity_id,since));
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		AccumulateInfo info = {};
		fill_map_activity_accumulate_info(info, *it);
		ret.emplace_back(std::move(info));
	}
}

void MapActivityAccumulateMap::insert(AccumulateInfo info){
	PROFILE_ME;

	const auto map_activity_accumulate_map = g_map_activity_accumulate_map.lock();
	if(!map_activity_accumulate_map){
		LOG_EMPERY_CENTER_WARNING("map activity accumulate map is not loaded.");
		return;
	}
	const auto it = map_activity_accumulate_map->find<0>(std::make_pair(info.account_uuid, std::make_pair(info.activity_id,info.avaliable_since)));
	if(it != map_activity_accumulate_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("map activity accumulate has exist: account_uuid = ", info.account_uuid, ", activity_id = ", info.activity_id.get());
		return;
	}
	const auto obj = boost::make_shared<MySql::Center_MapActivityAccumulate>(info.account_uuid.get(), info.activity_id.get(),
		info.avaliable_since, info.avaliable_util, info.accumulate_value);
	obj->async_save(true);
	map_activity_accumulate_map->insert(MapActivityAccumulateElement(std::move(obj)));
}

void MapActivityAccumulateMap::update(AccumulateInfo info,bool throws_if_not_exists){
	PROFILE_ME;

	const auto map_activity_accumulate_map = g_map_activity_accumulate_map.lock();
	if(!map_activity_accumulate_map){
		LOG_EMPERY_CENTER_WARNING("map activity accumulate map is not loaded.");
		return;
	}
	const auto it = map_activity_accumulate_map->find<0>(std::make_pair(info.account_uuid, std::make_pair(info.activity_id,info.avaliable_since)));
	if(it == map_activity_accumulate_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("map activity accumulate not found: account_uuid = ", info.account_uuid, ", activity_id = ", info.activity_id.get());
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("map activity not found"));
		}
		return;
	}
	auto &obj = (*it).obj;
	obj->set_avaliable_since(info.avaliable_since);
	obj->set_avaliable_util(info.avaliable_util);
	obj->set_accumulate_value(info.accumulate_value);
}

void MapActivityRankMap::get_recent_rank_list(MapActivityId activity_id, std::uint64_t settle_date,std::vector<MapActivityRankInfo> &ret){
	PROFILE_ME;

	const auto map_activity_rank_map = g_map_activity_rank_map.lock();
	if(!map_activity_rank_map){
		LOG_EMPERY_CENTER_WARNING("map activity rank map is not loaded.");
		return;
	}
	const auto range = map_activity_rank_map->equal_range<1>(std::make_pair(activity_id, settle_date));
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		MapActivityRankInfo info = {};
		fill_map_activity_rank_info(info, *it);
		ret.emplace_back(info);
	}
}
void MapActivityRankMap::insert(MapActivityRankInfo info){
	PROFILE_ME;

	const auto map_activity_rank_map = g_map_activity_rank_map.lock();
	if(!map_activity_rank_map){
		LOG_EMPERY_CENTER_WARNING("map activity rank map is not loaded.");
		return;
	}
	const auto it = map_activity_rank_map->find<0>(std::make_pair(info.account_uuid,std::make_pair(info.activity_id, info.settle_date)));
	if(it != map_activity_rank_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("map activity rank has exist: account_uuid = ", info.account_uuid, ", activity_id = ", info.activity_id.get(), " settle_date:",info.settle_date);
		return;
	}
	const auto obj = boost::make_shared<MySql::Center_MapActivityRank>(info.account_uuid.get(), info.activity_id.get(),
		info.settle_date, info.rank, info.accumulate_value,info.process_date);
	obj->async_save(true);
	map_activity_rank_map->insert(MapActivityRankElement(std::move(obj)));
}

WorldActivityMap::WorldActivityInfo WorldActivityMap::get(Coord coord, WorldActivityId activity_id,std::uint64_t since){
	PROFILE_ME;

	WorldActivityInfo info = {};
	const auto world_activity_map = g_world_activity_map.lock();
	if(!world_activity_map){
		LOG_EMPERY_CENTER_WARNING("world activity  map is not loaded.");
		return info;
	}
	const auto it = world_activity_map->find<0>(std::make_pair(activity_id,std::make_pair(since,coord)));
	if(it == world_activity_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("world activity  map  not found: coord = ", coord, ", activity_id = ", activity_id.get());
		return info;
	}
	fill_world_activity_info(info, *it);
	return info;
}

void WorldActivityMap::get_recent_world_activity(Coord coord,std::uint64_t since,std::vector<WorldActivityInfo> &ret){
	PROFILE_ME;

	const auto world_activity_map = g_world_activity_map.lock();
	if(!world_activity_map){
		LOG_EMPERY_CENTER_WARNING("world activity  map is not loaded.");
		return;
	}
	const auto range = world_activity_map->equal_range<1>(std::make_pair(since,coord));
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		WorldActivityInfo info = {};
		fill_world_activity_info(info, *it);
		ret.emplace_back(std::move(info));
	}
}

void WorldActivityMap::insert(WorldActivityInfo info){
	PROFILE_ME;

	const auto world_activity_map = g_world_activity_map.lock();
	if(!world_activity_map){
		LOG_EMPERY_CENTER_WARNING("world activity  map is not loaded.");
		return;
	}
	const auto it = world_activity_map->find<0>(std::make_pair(info.activity_id,std::make_pair(info.since, info.cluster_coord)));
	if(it != world_activity_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("world activity accumulate has exist: cluster coord = ", info.cluster_coord, ", activity_id = ", info.activity_id.get(), " ,since = ",info.since);
		return;
	}
	const auto obj = boost::make_shared<MySql::Center_MapWorldActivity>(info.activity_id.get(), info.cluster_coord.x(),info.cluster_coord.y(),
		info.since, info.sub_since, info.sub_until,info.accumulate_value,info.finish);
	obj->async_save(true);
	world_activity_map->insert(WorldActivityElement(std::move(obj)));
}

void WorldActivityMap::update(WorldActivityInfo info,bool throws_if_not_exists){
	PROFILE_ME;

	const auto world_activity_map = g_world_activity_map.lock();
	if(!world_activity_map){
		LOG_EMPERY_CENTER_WARNING("world activity  map is not loaded.");
		return;
	}
	const auto it = world_activity_map->find<0>(std::make_pair(info.activity_id,std::make_pair(info.since, info.cluster_coord)));
	if(it == world_activity_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("world activity not found: cluster coord  = ", info.cluster_coord, ", activity_id = ", info.activity_id.get(), ", since = ",info.since);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("world activity not found"));
		}
		return;
	}
	auto &obj = (*it).obj;
	obj->set_sub_until(info.sub_until);
	obj->set_accumulate_value(info.accumulate_value);
	obj->set_finish(info.finish);
}

WorldActivityAccumulateMap::WorldActivityAccumulateInfo WorldActivityAccumulateMap::get(AccountUuid account_uuid,Coord coord, WorldActivityId activity_id,std::uint64_t since){
	PROFILE_ME;

	WorldActivityAccumulateInfo info = {};
	const auto world_activity_accumulate_map = g_world_activity_accumulate_map.lock();
	if(!world_activity_accumulate_map){
		LOG_EMPERY_CENTER_WARNING("world activity accumulate map is not loaded.");
		return info;
	}
	const auto it = world_activity_accumulate_map->find<0>(std::make_pair(std::make_pair(account_uuid,activity_id),std::make_pair(since,coord)));
	if(it == world_activity_accumulate_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("world activity accumulate not found: account_uuid = ", account_uuid, ", activity_id = ", activity_id.get(),", since = ", since, ", coord = ",coord);
		return info;
	}
	fill_world_activity_accumluate_info(info, *it);
	return info;
}

void WorldActivityAccumulateMap::get_recent_activity_accumulate_info(Coord coord,std::uint64_t since,std::vector<WorldActivityAccumulateInfo> &ret){
	PROFILE_ME;
	
	const auto world_activity_accumulate_map = g_world_activity_accumulate_map.lock();
	if(!world_activity_accumulate_map){
		LOG_EMPERY_CENTER_WARNING("world activity accumulate map is not loaded.");
		return;
	}
	const auto range = world_activity_accumulate_map->equal_range<1>(std::make_pair(since,coord));
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		WorldActivityAccumulateInfo info = {};
		fill_world_activity_accumluate_info(info, *it);
		ret.emplace_back(std::move(info));
	}
}

void WorldActivityAccumulateMap::update(WorldActivityAccumulateInfo info){
	PROFILE_ME;
	
	const auto world_activity_accumulate_map = g_world_activity_accumulate_map.lock();
	if(!world_activity_accumulate_map){
		LOG_EMPERY_CENTER_WARNING("world activity accumulate map is not loaded.");
		return;
	}
	const auto it = world_activity_accumulate_map->find<0>(std::make_pair(std::make_pair(info.account_uuid,info.activity_id),std::make_pair(info.since,info.cluster_coord)));
	if(it == world_activity_accumulate_map->end<0>()){
		const auto obj = boost::make_shared<MySql::Center_MapWorldActivityAccumulate>(info.account_uuid.get(),info.activity_id.get(),info.cluster_coord.x(),info.cluster_coord.y(),
		info.since, info.accumulate_value);
		obj->async_save(true);
		world_activity_accumulate_map->insert(WorldActivityAccumulateElement(std::move(obj)));
		return;
	}
	auto &obj = (*it).obj;
	obj->set_accumulate_value(info.accumulate_value);
}

void WorldActivityAccumulateMap::get_recent_world_activity_account(Coord coord, WorldActivityId activity_id,std::uint64_t since,std::vector<AccountUuid> &ret){
	PROFILE_ME;
	
	const auto world_activity_accumulate_map = g_world_activity_accumulate_map.lock();
	if(!world_activity_accumulate_map){
		LOG_EMPERY_CENTER_WARNING("world activity accumulate map is not loaded.");
		return;
	}
	const auto range = world_activity_accumulate_map->equal_range<2>(std::make_pair(activity_id,std::make_pair(since,coord)));
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(AccountUuid((*it).obj->get_account_uuid()));
	}
}

WorldActivityRankMap::WorldActivityRankInfo WorldActivityRankMap::get_account_rank(AccountUuid account_uuid, Coord coord, std::uint64_t since){
	PROFILE_ME;
	
	WorldActivityRankInfo info = { };
	const auto world_activity_rank_map = g_world_activity_rank_map.lock();
	if(!world_activity_rank_map){
		LOG_EMPERY_CENTER_WARNING("world activity rank map is not loaded.");
		return info;
	}
	const auto it = world_activity_rank_map->find<0>(std::make_pair(account_uuid,std::make_pair(coord,since)));
	if(it != world_activity_rank_map->end<0>()){
		fill_world_activity_rank_info(info, *it);
		return info;
	}
	return info;
}

void WorldActivityRankMap::get_recent_rank_list(Coord coord, std::uint64_t since,std::uint64_t rank_threshold,std::vector<WorldActivityRankInfo> &ret){
	PROFILE_ME;
	
	const auto world_activity_rank_map = g_world_activity_rank_map.lock();
	if(!world_activity_rank_map){
		LOG_EMPERY_CENTER_WARNING("world activity rank map is not loaded.");
		return;
	}
	const auto range = world_activity_rank_map->equal_range<1>(std::make_pair(coord, since));
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if((*it).obj->get_rank() > rank_threshold){
			continue;
		}
		WorldActivityRankInfo info = { };
		fill_world_activity_rank_info(info, *it);
		ret.emplace_back(info);
	}
}
void WorldActivityRankMap::insert(WorldActivityRankInfo info){
	PROFILE_ME;
	
	const auto world_activity_rank_map = g_world_activity_rank_map.lock();
	if(!world_activity_rank_map){
		LOG_EMPERY_CENTER_WARNING("world activity rank map is not loaded.");
		return;
	}
	const auto it = world_activity_rank_map->find<0>(std::make_pair(info.account_uuid,std::make_pair(info.cluster_coord,info.since)));
	if(it != world_activity_rank_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("world activity rank has exist: account_uuid = ",info.account_uuid," cluster coord = ", info.cluster_coord, ", since = ", info.since);
		return;
	}
	const auto obj = boost::make_shared<MySql::Center_MapWorldActivityRank>(info.account_uuid.get(),info.cluster_coord.x(),info.cluster_coord.y(),
		info.since, info.rank, info.accumulate_value,info.process_date);
	obj->async_save(true);
	world_activity_rank_map->insert(WorldActivityRankElement(std::move(obj)));
}

std::uint64_t WorldActivityRankMap::get_pro_rank_time(Coord coord, std::uint64_t since){
	PROFILE_ME;
	
	const auto world_activity_rank_map = g_world_activity_rank_map.lock();
	if(!world_activity_rank_map){
		LOG_EMPERY_CENTER_WARNING("world activity rank map is not loaded.");
		return 0;
	}
	
	std::uint64_t pro_time = 0;
	const auto max_it = world_activity_rank_map->upper_bound<1>(std::make_pair(coord, since));
	for(auto it = world_activity_rank_map->begin<1>(); it != max_it; ++it){
		const auto since = (*it).obj->get_since();
		const auto cluster_coord = Coord((*it).obj->get_cluster_x(),(*it).obj->get_cluster_y());
		if((cluster_coord == coord) && (pro_time < since) ){
			pro_time = since;
		}
	}
	return pro_time;
}

WorldActivityBossMap::WorldActivityBossInfo WorldActivityBossMap::get(Coord cluster_coord, std::uint64_t since){
	PROFILE_ME;

	WorldActivityBossInfo info = {};
	const auto world_activity_boss_map = g_world_activity_boss_map.lock();
	if(!world_activity_boss_map){
		LOG_EMPERY_CENTER_WARNING("world activity boss map is not loaded.");
		return info;
	}
	const auto it = world_activity_boss_map->find<0>(std::make_pair(cluster_coord,since));
	if(it == world_activity_boss_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("world activity boss not found: coord = ", cluster_coord, ", since = ", since);
		return info;
	}
	fill_world_activity_boss_info(info, *it);
	return info;
}
void WorldActivityBossMap::update(WorldActivityBossInfo info){
	PROFILE_ME;
	
	const auto world_activity_boss_map = g_world_activity_boss_map.lock();
	if(!world_activity_boss_map){
		LOG_EMPERY_CENTER_WARNING("world activity boss map is not loaded.");
		return;
	}
	const auto it = world_activity_boss_map->find<0>(std::make_pair(info.cluster_coord,info.since));
	if(it == world_activity_boss_map->end<0>()){
		const auto obj = boost::make_shared<MySql::Center_MapWorldActivityBoss>(info.cluster_coord.x(),info.cluster_coord.y(),info.since,info.boss_uuid.get(),info.create_date,info.delete_date,info.hp_total,info.hp_die);
		obj->async_save(true);
		world_activity_boss_map->insert(WorldActivityBossElement(std::move(obj)));
		return;
	}
	auto &obj = (*it).obj;
	obj->set_delete_date(info.delete_date);
	obj->set_hp_total(info.hp_total);
	obj->set_hp_die(info.hp_die);
}

}
