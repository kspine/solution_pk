#include "../precompiled.hpp"
#include "map_activity_accumulate_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include "../mysql/map_activity_accumulate.hpp"
#include "../mysql/map_activity_rank.hpp"


namespace EmperyCenter {

namespace {
	struct MapActivityAccumulateElement {
		boost::shared_ptr<MySql::Center_MapActivityAccumulate> obj;
		std::pair<AccountUuid, MapActivityId> account_activity_pair;
		std::uint64_t    		avaliable_util;

		MapActivityAccumulateElement(boost::shared_ptr<MySql::Center_MapActivityAccumulate> obj_)
			:obj(std::move(obj_)),account_activity_pair(AccountUuid(obj->get_account_uuid()), MapActivityId(obj->get_map_activity_id())), avaliable_util(obj->get_avaliable_util())
		{
		}
	};

	MULTI_INDEX_MAP(MapActivityAccumulateContainer, MapActivityAccumulateElement,
		UNIQUE_MEMBER_INDEX(account_activity_pair)
		MULTI_MEMBER_INDEX(avaliable_util)
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

	boost::weak_ptr<MapActivityAccumulateContainer> g_map_activity_accumulate_map;
	boost::weak_ptr<MapActivityRankContainer> g_map_activity_rank_map;

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

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("map activity accumulate gc timer: now = ", now);

		const auto map_activity_accumulate_map = g_map_activity_accumulate_map.lock();
		if(!map_activity_accumulate_map){
			return;
		}

		for(;;){
			const auto it = map_activity_accumulate_map->begin<1>();
			if(it == map_activity_accumulate_map->end<1>()){
				break;
			}
			if(now > it->avaliable_util){
				map_activity_accumulate_map->erase<1>(it);
			}
		}
		std::stringstream ss;
		ss << "DELETE FROM `Center_MapActivityAccumulate` WHERE avaliable_util < " << now;
		Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_MapActivityAccumulate",ss.str());
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
		/*
		const auto gc_interval = get_config<std::uint64_t>("war_status_refresh_interval", 60000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
		*/
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
	}
}

MapActivityAccumulateMap::AccumulateInfo MapActivityAccumulateMap::get(AccountUuid account_uuid, MapActivityId activity_id){
	PROFILE_ME;

	AccumulateInfo info = {};
	const auto map_activity_accumulate_map = g_map_activity_accumulate_map.lock();
	if(!map_activity_accumulate_map){
		LOG_EMPERY_CENTER_WARNING("map activity accumulate map is not loaded.");
		return info;
	}
	const auto it = map_activity_accumulate_map->find<0>(std::make_pair(account_uuid, activity_id));
	if(it == map_activity_accumulate_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("map activity accumulate not found: account_uuid = ", account_uuid, ", activity_id = ", activity_id.get());
		return info;
	}
	fill_map_activity_accumulate_info(info, *it);
	return info;
}

void MapActivityAccumulateMap::insert(AccumulateInfo info){
	PROFILE_ME;

	const auto map_activity_accumulate_map = g_map_activity_accumulate_map.lock();
	if(!map_activity_accumulate_map){
		LOG_EMPERY_CENTER_WARNING("map activity accumulate map is not loaded.");
		return;
	}
	const auto it = map_activity_accumulate_map->find<0>(std::make_pair(info.account_uuid, info.activity_id));
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
	const auto it = map_activity_accumulate_map->find<0>(std::make_pair(info.account_uuid, info.activity_id));
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
}
