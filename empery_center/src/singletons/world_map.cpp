#include "../precompiled.hpp"
#include "world_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/async_job.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "player_session_map.hpp"
#include "account_map.hpp"
#include "../msg/sc_map.hpp"
#include "../msg/kill.hpp"
#include "../data/global.hpp"
#include "../data/map.hpp"
#include "../map_cell.hpp"
#include "../mysql/map_cell.hpp"
#include "../map_object.hpp"
#include "../map_object_type_ids.hpp"
#include "../mysql/map_object.hpp"
#include "../mysql/defense_building.hpp"
#include "../mysql/castle.hpp"
#include "../defense_building.hpp"
#include "../castle.hpp"
#include "../strategic_resource.hpp"
#include "../mysql/strategic_resource.hpp"
#include "../resource_crate.hpp"
#include "../mysql/resource_crate.hpp"
#include "../map_event_block.hpp"
#include "../mysql/map_event.hpp"
#include "../player_session.hpp"
#include "../cluster_session.hpp"
#include "../map_utilities_center.hpp"
#include "../map_event_block.hpp"
#include "../account.hpp"
#include "../account_attribute_ids.hpp"
#include "controller_client.hpp"
#include "../msg/st_map.hpp"
#include "global_status.hpp"

namespace EmperyCenter {

namespace {
	enum : unsigned {
		MAP_WIDTH          = 600,
		MAP_HEIGHT         = 640,

		SECTOR_SIDE_LENGTH = 32,

		EVENT_BLOCK_WIDTH  = MapEventBlock::BLOCK_WIDTH,
		EVENT_BLOCK_HEIGHT = MapEventBlock::BLOCK_HEIGHT,
	};

	inline Coord get_sector_coord_from_world_coord(Coord coord){
		const auto mask_x = coord.x() >> 63;
		const auto mask_y = coord.y() >> 63;

		const auto sector_x = ((coord.x() ^ mask_x) / SECTOR_SIDE_LENGTH ^ mask_x) * SECTOR_SIDE_LENGTH;
		const auto sector_y = ((coord.y() ^ mask_y) / SECTOR_SIDE_LENGTH ^ mask_y) * SECTOR_SIDE_LENGTH;

		return Coord(sector_x, sector_y);
	}

	inline Coord get_event_block_coord_from_world_coord(Coord coord){
		const auto mask_x = coord.x() >> 63;
		const auto mask_y = coord.y() >> 63;

		const auto block_x = ((coord.x() ^ mask_x) / EVENT_BLOCK_WIDTH  ^ mask_x) * EVENT_BLOCK_WIDTH;
		const auto block_y = ((coord.y() ^ mask_y) / EVENT_BLOCK_HEIGHT ^ mask_y) * EVENT_BLOCK_HEIGHT;

		return Coord(block_x, block_y);
	}

	inline Coord get_cluster_coord_from_world_coord(Coord coord){
		const auto mask_x = coord.x() >> 63;
		const auto mask_y = coord.y() >> 63;

		const auto cluster_x = ((coord.x() ^ mask_x) / MAP_WIDTH  ^ mask_x) * MAP_WIDTH;
		const auto cluster_y = ((coord.y() ^ mask_y) / MAP_HEIGHT ^ mask_y) * MAP_HEIGHT;

		return Coord(cluster_x, cluster_y);
	}

	struct PlayerViewElement {
		boost::weak_ptr<PlayerSession> session;
		Coord sector_coord;

		PlayerViewElement(boost::weak_ptr<PlayerSession> session_, Coord sector_coord_)
			: session(std::move(session_)), sector_coord(sector_coord_)
		{
		}
	};

	struct MapCellElement {
		boost::shared_ptr<MapCell> map_cell;

		Coord coord;
		MapObjectUuid parent_object_uuid;
		MapObjectUuid occupier_object_uuid;
		bool auto_update;

		explicit MapCellElement(boost::shared_ptr<MapCell> map_cell_)
			: map_cell(std::move(map_cell_))
			, coord(map_cell->get_coord()), parent_object_uuid(map_cell->get_parent_object_uuid())
			, occupier_object_uuid(map_cell->get_occupier_object_uuid())
			, auto_update(map_cell->should_auto_update())
		{
		}
	};

	MULTI_INDEX_MAP(MapCellContainer, MapCellElement,
		UNIQUE_MEMBER_INDEX(coord)
		MULTI_MEMBER_INDEX(parent_object_uuid)
		MULTI_MEMBER_INDEX(occupier_object_uuid)
		MULTI_MEMBER_INDEX(auto_update)
	)

	boost::weak_ptr<MapCellContainer> g_map_cell_map;

	void map_cell_occupation_refresh_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Map cell occupation refresh timer: now = ", now);

		const auto map_cell_map = g_map_cell_map.lock();
		if(!map_cell_map){
			return;
		}

		std::vector<boost::shared_ptr<MapCell>> map_cells_to_pump;
		map_cells_to_pump.reserve(map_cell_map->size());
		for(auto it = map_cell_map->upper_bound<2>(MapObjectUuid()); it != map_cell_map->end<2>(); ++it){
			const auto &map_cell = it->map_cell;
			if(map_cell->is_virtually_removed()){
				continue;
			}
			map_cells_to_pump.emplace_back(map_cell);
		}
		for(auto it = map_cells_to_pump.begin(); it != map_cells_to_pump.end(); ++it){
			const auto &map_cell = *it;
			try {
				map_cell->pump_status();
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}
		}

		Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_MapEvent",
			"DELETE QUICK `e`.*"
			"  FROM `Center_MapEvent` AS `e` "
			"  WHERE `e`.`expiry_time` = '0000-00-00 00:00:00'");
	}

	inline MapObjectUuid get_garrisoning_object_uuid(const boost::shared_ptr<MapObject> map_object){
		PROFILE_ME;

		const auto map_object_type_id = map_object->get_map_object_type_id();
		if(map_object_type_id != MapObjectTypeIds::ID_BATTLE_BUNKER){
			return { };
		}
		const auto bunker = boost::dynamic_pointer_cast<DefenseBuilding>(map_object);
		if(!bunker){
			return { };
		}
		return bunker->get_garrisoning_object_uuid();
	}

	struct MapObjectElement {
		boost::shared_ptr<MapObject> map_object;

		MapObjectUuid map_object_uuid;
		Coord coord;
		AccountUuid owner_uuid;
		MapObjectUuid parent_object_uuid;
		MapObjectUuid garrisoning_object_uuid;
		bool auto_update;

		explicit MapObjectElement(boost::shared_ptr<MapObject> map_object_)
			: map_object(std::move(map_object_))
			, map_object_uuid(map_object->get_map_object_uuid()), coord(map_object->get_coord())
			, owner_uuid(map_object->get_owner_uuid()), parent_object_uuid(map_object->get_parent_object_uuid())
			, garrisoning_object_uuid(get_garrisoning_object_uuid(map_object))
			, auto_update(map_object->should_auto_update())
		{
		}
	};

	MULTI_INDEX_MAP(MapObjectContainer, MapObjectElement,
		UNIQUE_MEMBER_INDEX(map_object_uuid)
		MULTI_MEMBER_INDEX(coord)
		MULTI_MEMBER_INDEX(owner_uuid)
		MULTI_MEMBER_INDEX(parent_object_uuid)
		MULTI_MEMBER_INDEX(garrisoning_object_uuid)
		MULTI_MEMBER_INDEX(auto_update)
	)

	boost::weak_ptr<MapObjectContainer> g_map_object_map;

	void map_object_refresh_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Map object refresh timer: now = ", now);

		const auto map_object_map = g_map_object_map.lock();
		if(!map_object_map){
			return;
		}

		std::vector<boost::shared_ptr<MapObject>> map_objects_to_pump;
		const auto range = std::make_pair(map_object_map->upper_bound<5>(false), map_object_map->end<5>());
		map_objects_to_pump.reserve(static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			const auto &map_object = it->map_object;
			map_objects_to_pump.emplace_back(map_object);
		}
		for(auto it = map_objects_to_pump.begin(); it != map_objects_to_pump.end(); ++it){
			const auto &map_object = *it;
			try {
				map_object->pump_status();
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}
		}

		Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_MapObject",
			"DELETE QUICK `m`.*, `a`.*, `d`.* "
			"  FROM `Center_MapObject` AS `m` "
			"    LEFT JOIN `Center_MapObjectAttribute` AS `a` "
			"      ON `m`.`map_object_uuid` = `a`.`map_object_uuid` "
			"    LEFT JOIN `Center_DefenseBuilding` AS `d` "
			"      ON `m`.`map_object_uuid` = `d`.`map_object_uuid` "
			"    LEFT JOIN `Center_MapObjectBuff` AS `b` "
			"      ON `m`.`map_object_uuid` = `b`.`map_object_uuid` "
			"  WHERE `m`.`deleted` > 0");
	}

	void castle_activity_check_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Castle activity check timer: now = ", now);

		const auto map_object_map = g_map_object_map.lock();
		if(!map_object_map){
			return;
		}

		const auto castle_hang_up_inactive_minutes = Data::Global::as_unsigned(Data::Global::SLOT_CASTLE_HANG_UP_INACTIVE_MINUTES);
		const auto utc_now = Poseidon::get_utc_time();

		std::vector<boost::shared_ptr<Castle>> castles_to_check;
		castles_to_check.reserve(map_object_map->size() / 10);
		for(auto it = map_object_map->begin<0>(); it != map_object_map->end<0>(); ++it){
			const auto &map_object = it->map_object;
			const auto map_object_type_id = map_object->get_map_object_type_id();
			if(map_object_type_id != MapObjectTypeIds::ID_CASTLE){
				continue;
			}
			if(map_object->is_virtually_removed()){
				continue;
			}
			if(map_object->is_garrisoned()){
				continue;
			}
			auto castle = boost::dynamic_pointer_cast<Castle>(map_object);
			if(!castle){
				continue;
			}
			castles_to_check.emplace_back(std::move(castle));
		}
		for(auto it = castles_to_check.begin(); it != castles_to_check.end(); ++it){
			const auto &castle = *it;
			try {
				const auto owner_account = AccountMap::require(castle->get_owner_uuid());
				std::uint64_t last_logged_out_time = 0;
				const auto &last_logged_in_time_str = owner_account->get_attribute(AccountAttributeIds::ID_LAST_LOGGED_IN_TIME);
				if(last_logged_in_time_str.empty()){
					last_logged_out_time = owner_account->get_created_time();
				} else {
					const auto &last_logged_out_time_str = owner_account->get_attribute(AccountAttributeIds::ID_LAST_LOGGED_OUT_TIME);
					if(last_logged_out_time_str.empty()){
						last_logged_out_time = UINT64_MAX;
					} else {
						last_logged_out_time = boost::lexical_cast<std::uint64_t>(last_logged_out_time_str);
					}
				}
				LOG_EMPERY_CENTER_TRACE("$@ Checking active account: account_uuid = ", owner_account->get_account_uuid(),
					", last_logged_out_time = ", last_logged_out_time);
				if(saturated_sub(utc_now, last_logged_out_time) / 60000 < castle_hang_up_inactive_minutes){
					continue;
				}
				LOG_EMPERY_CENTER_INFO("Hang up inactive castle: map_object_uuid = ", castle->get_map_object_uuid());
				async_hang_up_castle(castle);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}
		}
	}

	struct StrategicResourceElement {
		boost::shared_ptr<StrategicResource> strategic_resource;

		Coord coord;
		std::uint64_t expiry_time;

		explicit StrategicResourceElement(boost::shared_ptr<StrategicResource> strategic_resource_)
			: strategic_resource(std::move(strategic_resource_))
			, coord(strategic_resource->get_coord())
			, expiry_time(strategic_resource->get_expiry_time())
		{
		}
	};

	MULTI_INDEX_MAP(StrategicResourceContainer, StrategicResourceElement,
		UNIQUE_MEMBER_INDEX(coord)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<StrategicResourceContainer> g_strategic_resource_map;

	void strategic_resource_refresh_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Map object refresh timer: now = ", now);

		const auto strategic_resource_map = g_strategic_resource_map.lock();
		if(!strategic_resource_map){
			return;
		}

		const auto utc_now = Poseidon::get_utc_time();
		const auto range = std::make_pair(strategic_resource_map->begin<1>(), strategic_resource_map->upper_bound<1>(utc_now));

		std::vector<boost::shared_ptr<StrategicResource>> resources_to_delete;
		resources_to_delete.reserve(static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			resources_to_delete.emplace_back(it->strategic_resource);
		}
		for(auto it = resources_to_delete.begin(); it != resources_to_delete.end(); ++it){
			const auto &strategic_resource = *it;
			LOG_EMPERY_CENTER_DEBUG("Reclaiming strategic resource: coord = ", strategic_resource->get_coord());
			strategic_resource->delete_from_game();
		}

		Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_StrategicResource",
			"DELETE QUICK `r`.* "
			"  FROM `Center_StrategicResource` AS `r` "
			"  WHERE `r`.`resource_amount` = 0");
	}

	struct MapEventBlockElement {
		boost::shared_ptr<MapEventBlock> map_event_block;

		Coord block_coord;

		explicit MapEventBlockElement(boost::shared_ptr<MapEventBlock> map_event_block_)
			: map_event_block(std::move(map_event_block_))
			, block_coord(get_event_block_coord_from_world_coord(map_event_block->get_block_coord()))
		{
		}
	};

	MULTI_INDEX_MAP(MapEventBlockContainer, MapEventBlockElement,
		UNIQUE_MEMBER_INDEX(block_coord)
	)

	boost::weak_ptr<MapEventBlockContainer> g_map_event_block_map;

	void map_event_block_refresh_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Map event block refresh timer: now = ", now);

		const auto map_event_block_map = g_map_event_block_map.lock();
		if(!map_event_block_map){
			return;
		}

		for(auto it = map_event_block_map->begin<0>(); it != map_event_block_map->end<0>(); ++it){
			const auto &map_event_block = it->map_event_block;
			try {
				map_event_block->pump_status();
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}
		}
	}

	struct ResourceCrateElement {
		boost::shared_ptr<ResourceCrate> resource_crate;

		ResourceCrateUuid resource_crate_uuid;
		Coord coord;
		std::uint64_t expiry_time;

		explicit ResourceCrateElement(boost::shared_ptr<ResourceCrate> resource_crate_)
			: resource_crate(std::move(resource_crate_))
			, resource_crate_uuid(resource_crate->get_resource_crate_uuid()), coord(resource_crate->get_coord())
			, expiry_time(resource_crate->get_expiry_time())
		{
		}
	};

	MULTI_INDEX_MAP(ResourceCrateContainer, ResourceCrateElement,
		UNIQUE_MEMBER_INDEX(resource_crate_uuid)
		MULTI_MEMBER_INDEX(coord)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<ResourceCrateContainer> g_resource_crate_map;

	void resource_crate_refresh_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Resource crate refresh timer: now = ", now);

		const auto resource_crate_map = g_resource_crate_map.lock();
		if(!resource_crate_map){
			return;
		}

		const auto utc_now = Poseidon::get_utc_time();
		const auto range = std::make_pair(resource_crate_map->begin<2>(), resource_crate_map->upper_bound<2>(utc_now));

		std::vector<boost::shared_ptr<ResourceCrate>> crates_to_delete;
		crates_to_delete.reserve(static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			crates_to_delete.emplace_back(it->resource_crate);
		}
		for(auto it = crates_to_delete.begin(); it != crates_to_delete.end(); ++it){
			const auto &resource_crate = *it;
			LOG_EMPERY_CENTER_DEBUG("Reclaiming resource crate: resource_crate_uuid = ", resource_crate->get_resource_crate_uuid());
			resource_crate->delete_from_game();
		}

		Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_ResourceCrate",
			"DELETE QUICK `c`.* "
			"  FROM `Center_ResourceCrate` AS `c` "
			"  WHERE `c`.`amount_remaining` = 0");
	}

	MULTI_INDEX_MAP(PlayerViewContainer, PlayerViewElement,
		MULTI_MEMBER_INDEX(session)
		MULTI_MEMBER_INDEX(sector_coord)
	)

	boost::weak_ptr<PlayerViewContainer> g_player_view_map;

	struct ClusterElement {
		Coord cluster_coord;
		boost::weak_ptr<ClusterSession> cluster;

		mutable std::vector<Coord> cached_start_points;

		ClusterElement(Coord cluster_coord_, boost::weak_ptr<ClusterSession> cluster_)
			: cluster_coord(cluster_coord_), cluster(std::move(cluster_))
		{
		}
	};

	MULTI_INDEX_MAP(ClusterContainer, ClusterElement,
		UNIQUE_MEMBER_INDEX(cluster_coord)
		UNIQUE_MEMBER_INDEX(cluster)
	)

	boost::weak_ptr<ClusterContainer> g_cluster_map;

	using SynchronizationKey = std::pair<boost::weak_ptr<PlayerSession>, boost::weak_ptr<const void>>;
	using SynchronizationCallback = boost::function<void (const boost::shared_ptr<PlayerSession> &)>;
	using SynchronizationQueue = std::map<SynchronizationKey, SynchronizationCallback>;
	boost::weak_ptr<SynchronizationQueue> g_synchronization_queue;

	void commit_world_synchronization(const boost::shared_ptr<SynchronizationQueue> &synchronization_queue){
		PROFILE_ME;

		for(;;){
			const auto it = synchronization_queue->begin();
			if(it == synchronization_queue->end()){
				break;
			}
			const auto session = it->first.first.lock();
			if(session){
				try {
					it->second(session);
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
					session->shutdown(e.what());
				}
			}
			synchronization_queue->erase(it);
		}
	}

	MODULE_RAII_PRIORITY(handles, 5300){
		const auto map_cell_map = boost::make_shared<MapCellContainer>();
		g_map_cell_map = map_cell_map;
		handles.push(map_cell_map);

		const auto map_object_map = boost::make_shared<MapObjectContainer>();
		g_map_object_map = map_object_map;
		handles.push(map_object_map);

		const auto strategic_resource_map = boost::make_shared<StrategicResourceContainer>();
		g_strategic_resource_map = strategic_resource_map;
		handles.push(strategic_resource_map);

		const auto map_event_block_map = boost::make_shared<MapEventBlockContainer>();
		g_map_event_block_map = map_event_block_map;
		handles.push(map_event_block_map);

		const auto resource_crate_map = boost::make_shared<ResourceCrateContainer>();
		g_resource_crate_map = resource_crate_map;
		handles.push(resource_crate_map);

		const auto player_view_map = boost::make_shared<PlayerViewContainer>();
		g_player_view_map = player_view_map;
		handles.push(player_view_map);

		const auto cluster_map = boost::make_shared<ClusterContainer>();
		g_cluster_map = cluster_map;
		handles.push(cluster_map);

		const auto synchronization_queue = boost::make_shared<SynchronizationQueue>();
		g_synchronization_queue = synchronization_queue;
		handles.push(synchronization_queue);

		const auto map_cell_occupation_refresh_interval = get_config<std::uint64_t>("map_cell_occupation_refresh_interval", 30000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, map_cell_occupation_refresh_interval,
			std::bind(&map_cell_occupation_refresh_timer_proc, std::placeholders::_2));
		handles.push(timer);

		const auto map_object_refresh_interval = get_config<std::uint64_t>("map_object_refresh_interval", 300000);
		timer = Poseidon::TimerDaemon::register_timer(0, map_object_refresh_interval,
			std::bind(&map_object_refresh_timer_proc, std::placeholders::_2));
		handles.push(timer);

		const auto castle_activity_check_interval = get_config<std::uint64_t>("castle_activity_check_interval", 3600000);
		timer = Poseidon::TimerDaemon::register_timer(0, castle_activity_check_interval,
			std::bind(&castle_activity_check_proc, std::placeholders::_2));
		handles.push(timer);

		const auto strategic_resource_refresh_interval = get_config<std::uint64_t>("strategic_resource_refresh_interval", 300000);
		timer = Poseidon::TimerDaemon::register_timer(0, strategic_resource_refresh_interval,
			std::bind(&strategic_resource_refresh_timer_proc, std::placeholders::_2));
		handles.push(timer);

		const auto map_event_block_refresh_interval = get_config<std::uint64_t>("map_event_block_refresh_interval", 300000);
		timer = Poseidon::TimerDaemon::register_timer(0, map_event_block_refresh_interval,
			std::bind(&map_event_block_refresh_timer_proc, std::placeholders::_2));
		handles.push(timer);

		const auto resource_crate_refresh_interval = get_config<std::uint64_t>("resource_crate_refresh_interval", 300000);
		timer = Poseidon::TimerDaemon::register_timer(0, resource_crate_refresh_interval,
			std::bind(&resource_crate_refresh_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}

	boost::shared_ptr<MapCell> reload_map_cell_aux(boost::shared_ptr<MySql::Center_MapCell> obj){
		PROFILE_ME;

		std::deque<boost::shared_ptr<const Poseidon::JobPromise>> promises;

		const auto attributes = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapCellAttribute>>>();
		const auto buffs      = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapCellBuff>>>();

#define RELOAD_PART_(sink_, table_)	\
		{	\
			std::ostringstream oss;	\
			const auto x = obj->get_x();	\
			const auto y = obj->get_y();	\
			oss <<"SELECT * FROM `" #table_ "` WHERE `x` = " <<x <<" AND `y` = " <<y;	\
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(	\
				[sink_](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){	\
					auto obj = boost::make_shared<MySql:: table_ >();	\
					obj->fetch(conn);	\
					obj->enable_auto_saving();	\
					(sink_)->emplace_back(std::move(obj));	\
				}, #table_, oss.str());	\
			promises.emplace_back(std::move(promise));	\
		}
//=============================================================================
		RELOAD_PART_(attributes,         Center_MapCellAttribute)
		RELOAD_PART_(buffs,              Center_MapCellBuff)
//=============================================================================
		for(const auto &promise : promises){
			Poseidon::JobDispatcher::yield(promise, true);
		}
#undef RELOAD_PART_

		return boost::make_shared<MapCell>(std::move(obj), *attributes, *buffs);
	}
	boost::shared_ptr<MapObject> reload_map_object_aux(boost::shared_ptr<MySql::Center_MapObject> obj){
		PROFILE_ME;

		std::deque<boost::shared_ptr<const Poseidon::JobPromise>> promises;

		// MapObject
		const auto attributes         = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>>>();
		const auto buffs              = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapObjectBuff>>>();
		// DefenseBuilding
		const auto defense_objs       = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_DefenseBuilding>>>();
		// Castle
		const auto buildings          = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>>>();
		const auto techs              = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_CastleTech>>>();
		const auto resources          = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_CastleResource>>>();
		const auto soldiers           = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_CastleBattalion>>>();
		const auto soldier_production = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_CastleBattalionProduction>>>();
		const auto wounded_soldiers   = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_CastleWoundedSoldier>>>();
		const auto treatment          = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_CastleTreatment>>>();

#define RELOAD_PART_(sink_, table_)	\
		{	\
			std::ostringstream oss;	\
			const auto &map_object_uuid = obj->unlocked_get_map_object_uuid();	\
			oss <<"SELECT * FROM `" #table_ "` WHERE `map_object_uuid` = " <<Poseidon::MySql::UuidFormatter(map_object_uuid);	\
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(	\
				[sink_](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){	\
					auto obj = boost::make_shared<MySql:: table_ >();	\
					obj->fetch(conn);	\
					obj->enable_auto_saving();	\
					(sink_)->emplace_back(std::move(obj));	\
				}, #table_, oss.str());	\
			promises.emplace_back(std::move(promise));	\
		}
//=============================================================================
		switch(obj->get_map_object_type_id()){
		case MapObjectTypeIds::ID_CASTLE.get():
			RELOAD_PART_(buildings,          Center_CastleBuildingBase)
			RELOAD_PART_(techs,              Center_CastleTech)
			RELOAD_PART_(resources,          Center_CastleResource)
			RELOAD_PART_(soldiers,           Center_CastleBattalion)
			RELOAD_PART_(soldier_production, Center_CastleBattalionProduction)
			RELOAD_PART_(wounded_soldiers,   Center_CastleWoundedSoldier)
			RELOAD_PART_(treatment,          Center_CastleTreatment)
		case MapObjectTypeIds::ID_DEFENSE_TOWER.get():
		case MapObjectTypeIds::ID_BATTLE_BUNKER.get():
			RELOAD_PART_(defense_objs,       Center_DefenseBuilding)
		default:
			RELOAD_PART_(attributes,         Center_MapObjectAttribute)
			RELOAD_PART_(buffs,              Center_MapObjectBuff)
		}
//=============================================================================
		for(const auto &promise : promises){
			Poseidon::JobDispatcher::yield(promise, true);
		}
#undef RELOAD_PART_

		boost::shared_ptr<Castle> castle;

		switch(obj->get_map_object_type_id()){
		case MapObjectTypeIds::ID_CASTLE.get():
			castle = boost::make_shared<Castle>(std::move(obj), *attributes, *buffs, *defense_objs,
				*buildings, *techs, *resources, *soldiers, *soldier_production, *wounded_soldiers, *treatment);
			castle->check_init_buildings();
			castle->check_init_resources();
			return std::move(castle);
		case MapObjectTypeIds::ID_DEFENSE_TOWER.get():
		case MapObjectTypeIds::ID_BATTLE_BUNKER.get():
			return boost::make_shared<DefenseBuilding>(std::move(obj), *attributes, *buffs, *defense_objs);
		default:
			return boost::make_shared<MapObject>(std::move(obj), *attributes, *buffs);
		}
	}
	boost::shared_ptr<MapEventBlock> reload_map_event_block_aux(boost::shared_ptr<MySql::Center_MapEventBlock> obj){
		PROFILE_ME;

		std::deque<boost::shared_ptr<const Poseidon::JobPromise>> promises;

		const auto events = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapEvent>>>();

#define RELOAD_PART_(sink_, table_)	\
		{	\
			std::ostringstream oss;	\
			const auto block_x = obj->get_block_x();	\
			const auto block_y = obj->get_block_y();	\
			oss <<"SELECT * FROM `" #table_ "` WHERE " <<block_x <<" <= `x` AND `x` < " <<(block_x + EVENT_BLOCK_WIDTH)	\
			    <<"  AND " <<block_y <<" <= `y` AND `y` < " <<(block_y + EVENT_BLOCK_HEIGHT);	\
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(	\
				[sink_](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){	\
					auto obj = boost::make_shared<MySql:: table_ >();	\
					obj->fetch(conn);	\
					obj->enable_auto_saving();	\
					(sink_)->emplace_back(std::move(obj));	\
				}, #table_, oss.str());	\
			promises.emplace_back(std::move(promise));	\
		}
//=============================================================================
		RELOAD_PART_(events,             Center_MapEvent)
//=============================================================================
		for(const auto &promise : promises){
			Poseidon::JobDispatcher::yield(promise, true);
		}
#undef RELOAD_PART_

		return boost::make_shared<MapEventBlock>(std::move(obj), *events);
	}

	bool should_erase_map_object(const boost::shared_ptr<MapObject> &map_object){
		PROFILE_ME;

		const auto coord = map_object->get_coord();
		const auto cluster = WorldMap::get_cluster(coord);
		if(cluster){
			return false;
		}

		// 如果部队所属城堡还在这个地图上就不能删除。
		const auto parent_object_uuid = map_object->get_parent_object_uuid();
		if(parent_object_uuid){
			const auto parent_object = WorldMap::get_map_object(parent_object_uuid);
			if(parent_object){
				const auto parent_coord = parent_object->get_coord();
				const auto parent_cluster = WorldMap::get_cluster(parent_coord);
				if(parent_cluster){
					return false;
				}
			}
		}

		const auto owner_uuid = map_object->get_owner_uuid();
		const auto session = PlayerSessionMap::get(owner_uuid);
		if(session){
			return false;
		}

		return true;
	}

	template<typename T>
	void synchronize_map_object_or_other(const boost::shared_ptr<T> &ptr,
		const boost::shared_ptr<PlayerSession> &session, Coord old_coord, Coord new_coord)
	{
		ptr->synchronize_with_player(session);

		(void)old_coord;
		(void)new_coord;
	}
	void synchronize_map_object_or_other(const boost::shared_ptr<MapObject> &ptr,
		const boost::shared_ptr<PlayerSession> &session, Coord old_coord, Coord new_coord)
	{
		ptr->synchronize_with_player(session);

		if(old_coord != new_coord){
			const auto castle = boost::dynamic_pointer_cast<Castle>(ptr);
			if(castle){
				Msg::SC_MapCastleRelocation msg;
				msg.castle_uuid = castle->get_map_object_uuid().str();
				msg.level       = castle->get_level();
				msg.old_coord_x = old_coord.x();
				msg.old_coord_y = old_coord.y();
				msg.new_coord_x = new_coord.x();
				msg.new_coord_y = new_coord.y();
				session->send(msg);
			}
		}
	}

	template<typename T>
	void synchronize_with_all_players(const boost::shared_ptr<T> &ptr, Coord old_coord, Coord new_coord,
		const boost::shared_ptr<PlayerSession> &excluded_session) noexcept
	{
		PROFILE_ME;

		const auto player_view_map = g_player_view_map.lock();
		if(!player_view_map){
			return;
		}
		const auto synchronization_queue = g_synchronization_queue.lock();
		if(!synchronization_queue){
			return;
		}

		const auto synchronize_in_sector = [&](Coord sector_coord){
			const auto range = player_view_map->equal_range<1>(sector_coord);
			for(auto next = range.first, it = next; (next != range.second) && (++next, true); it = next){
				const auto session = it->session.lock();
				if(!session){
					player_view_map->erase<1>(it);
					continue;
				}
				if(session == excluded_session){
					continue;
				}
				const auto view = session->get_view();
				if(view.hit_test(new_coord)){
					try {
						if(synchronization_queue->empty()){
							Poseidon::enqueue_async_job(std::bind(&commit_world_synchronization, synchronization_queue));
						}
						auto key = SynchronizationKey(session, ptr);
						auto sit = synchronization_queue->find(key);
						if(sit == synchronization_queue->end()){
							sit = synchronization_queue->insert(
								std::make_pair(std::move(key),
									[=](const boost::shared_ptr<PlayerSession> &session){
										synchronize_map_object_or_other(ptr, session, old_coord, new_coord);
									})
								).first;
						}
					} catch(std::exception &e){
						LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
						session->shutdown(e.what());
					}
				}
			}
		};

		const auto old_sector_coord = get_sector_coord_from_world_coord(old_coord);
		synchronize_in_sector(old_sector_coord);

		const auto new_sector_coord = get_sector_coord_from_world_coord(new_coord);
		if(new_sector_coord != old_sector_coord){
			synchronize_in_sector(new_sector_coord);
		}
	}
	template<typename T>
	void synchronize_with_all_clusters(const boost::shared_ptr<T> &ptr, Coord old_coord, Coord new_coord) noexcept {
		PROFILE_ME;

		const auto cluster_map = g_cluster_map.lock();
		if(!cluster_map){
			return;
		}

		const auto synchronize_in_cluster = [&](Coord cluster_coord){
			const auto range = cluster_map->equal_range<0>(cluster_coord);
			for(auto next = range.first, it = next; (next != range.second) && (++next, true); it = next){
				const auto cluster = it->cluster.lock();
				if(!cluster){
					cluster_map->erase<0>(it);
					continue;
				}
				const auto scope = WorldMap::get_cluster_scope(it->cluster_coord);
				if(scope.hit_test(new_coord)){
					try {
						ptr->synchronize_with_cluster(cluster);
					} catch(std::exception &e){
						LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
						cluster->shutdown(e.what());
					}
				}
			}
		};

		const auto old_cluster_coord = get_cluster_coord_from_world_coord(old_coord);
		synchronize_in_cluster(old_cluster_coord);

		const auto new_cluster_coord = get_cluster_coord_from_world_coord(new_coord);
		if(new_cluster_coord != old_cluster_coord){
			synchronize_in_cluster(new_cluster_coord);
		}
	}
	void synchronize_with_controller(const boost::shared_ptr<MapObject> &map_object, Coord old_coord, Coord new_coord) noexcept {
		PROFILE_ME;

		(void)old_coord;

		boost::shared_ptr<ControllerClient> controller;
		try {
			controller = ControllerClient::require();
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
		if(!controller){
			LOG_EMPERY_CENTER_FATAL("Could not connect to controller server!");
			return;
		}

		const auto map_object_uuid = map_object->get_map_object_uuid();

		const auto castle = boost::dynamic_pointer_cast<Castle>(map_object);
		if(castle){
			try {
				Poseidon::MySqlDaemon::enqueue_for_waiting_for_all_async_operations();

				Msg::ST_MapInvalidateCastle msg;
				msg.map_object_uuid = map_object_uuid.str();
				msg.coord_x         = new_coord.x();
				msg.coord_y         = new_coord.y();
				controller->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}
		}

		if(map_object->is_virtually_removed()){
			try {
				Poseidon::MySqlDaemon::enqueue_for_waiting_for_all_async_operations();

				Msg::ST_MapRemoveMapObject msg;
				msg.map_object_uuid = map_object_uuid.str();
				controller->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
	}
}

boost::shared_ptr<MapCell> WorldMap::get_map_cell(Coord coord){
	PROFILE_ME;

	const auto map_cell_map = g_map_cell_map.lock();
	if(!map_cell_map){
		LOG_EMPERY_CENTER_WARNING("Map cell map not loaded.");
		return { };
	}

	const auto it = map_cell_map->find<0>(coord);
	if(it == map_cell_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Map cell not found: coord = ", coord);
		return { };
	}
	return it->map_cell;
}
boost::shared_ptr<MapCell> WorldMap::require_map_cell(Coord coord){
	PROFILE_ME;

	auto ret = get_map_cell(coord);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Map cell not found: coord = ", coord);
		DEBUG_THROW(Exception, sslit("Map cell not found"));
	}
	return ret;
}
void WorldMap::insert_map_cell(const boost::shared_ptr<MapCell> &map_cell){
	PROFILE_ME;

	const auto map_cell_map = g_map_cell_map.lock();
	if(!map_cell_map){
		LOG_EMPERY_CENTER_WARNING("Map cell map not loaded.");
		DEBUG_THROW(Exception, sslit("Map cell map not loaded"));
	}

	const auto coord = map_cell->get_coord();

	LOG_EMPERY_CENTER_TRACE("Inserting map cell: coord = ", coord);
	const auto result = map_cell_map->insert(MapCellElement(map_cell));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Map cell already exists: coord = ", coord);
		DEBUG_THROW(Exception, sslit("Map cell already exists"));
	}

	const auto owner_uuid = map_cell->get_owner_uuid();
	const auto session = PlayerSessionMap::get(owner_uuid);
	if(session){
		try {
			synchronize_map_cell_with_player(map_cell, session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
	synchronize_with_all_players(map_cell, coord, coord, session);
	synchronize_with_all_clusters(map_cell, coord, coord);
}
void WorldMap::update_map_cell(const boost::shared_ptr<MapCell> &map_cell, bool throws_if_not_exists){
	PROFILE_ME;

	const auto map_cell_map = g_map_cell_map.lock();
	if(!map_cell_map){
		LOG_EMPERY_CENTER_WARNING("Map cell map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map cell map not loaded"));
		}
		return;
	}

	const auto coord = map_cell->get_coord();

	const auto it = map_cell_map->find<0>(coord);
	if(it == map_cell_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Map cell not found: coord = ", coord);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map cell not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating map cell: coord = ", coord);
	map_cell_map->replace<0>(it, MapCellElement(map_cell));

	const auto owner_uuid = map_cell->get_owner_uuid();
	const auto session = PlayerSessionMap::get(owner_uuid);
	if(session){
		try {
			synchronize_map_cell_with_player(map_cell, session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
	synchronize_with_all_players(map_cell, coord, coord, session);
	synchronize_with_all_clusters(map_cell, coord, coord);
}

/*void WorldMap::get_all_map_cells(std::vector<boost::shared_ptr<MapCell>> &ret){
	PROFILE_ME;

	const auto map_cell_map = g_map_cell_map.lock();
	if(!map_cell_map){
		LOG_EMPERY_CENTER_WARNING("Map cell map not loaded.");
		return;
	}

	ret.reserve(ret.size() + map_cell_map->size());
	for(auto it = map_cell_map->begin<0>(); it != map_cell_map->end<0>(); ++it){
		ret.emplace_back(it->map_cell);
	}
}*/
void WorldMap::get_map_cells_by_parent_object(std::vector<boost::shared_ptr<MapCell>> &ret, MapObjectUuid parent_object_uuid){
	PROFILE_ME;

	const auto map_cell_map = g_map_cell_map.lock();
	if(!map_cell_map){
		LOG_EMPERY_CENTER_WARNING("Map cell map not loaded.");
		return;
	}

	const auto range = map_cell_map->equal_range<1>(parent_object_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->map_cell);
	}
}
void WorldMap::get_map_cells_by_occupier_object(std::vector<boost::shared_ptr<MapCell>> &ret, MapObjectUuid occupier_object_uuid){
	PROFILE_ME;

	const auto map_cell_map = g_map_cell_map.lock();
	if(!map_cell_map){
		LOG_EMPERY_CENTER_WARNING("Map cell map not loaded.");
		return;
	}

	const auto range = map_cell_map->equal_range<2>(occupier_object_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->map_cell);
	}
}
void WorldMap::get_map_cells_by_rectangle(std::vector<boost::shared_ptr<MapCell>> &ret, Rectangle rectangle){
	PROFILE_ME;

	const auto map_cell_map = g_map_cell_map.lock();
	if(!map_cell_map){
		LOG_EMPERY_CENTER_WARNING("Map cell map not loaded.");
		return;
	}

	auto x = rectangle.left();
	while(x < rectangle.right()){
		auto it = map_cell_map->lower_bound<0>(Coord(x, rectangle.bottom()));
		for(;;){
			if(it == map_cell_map->end<0>()){
				goto _exit_while;
			}
			if(it->coord.x() != x){
				x = it->coord.x();
				break;
			}
			if(it->coord.y() >= rectangle.top()){
				++x;
				break;
			}
			ret.emplace_back(it->map_cell);
			++it;
		}
	}
_exit_while:
	;
}

boost::shared_ptr<MapObject> WorldMap::get_map_object(MapObjectUuid map_object_uuid){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return { };
	}

	const auto it = map_object_map->find<0>(map_object_uuid);
	if(it == map_object_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Map object not found: map_object_uuid = ", map_object_uuid);
		return { };
	}
	return it->map_object;
}
boost::shared_ptr<MapObject> WorldMap::get_or_reload_map_object(MapObjectUuid map_object_uuid){
	PROFILE_ME;

	auto map_object = get_map_object(map_object_uuid);
	if(!map_object){
		map_object = forced_reload_map_object(map_object_uuid);
	}
	return map_object;
}
boost::shared_ptr<MapObject> WorldMap::forced_reload_map_object(MapObjectUuid map_object_uuid){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return { };
	}

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapObject>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_MapObject` WHERE `deleted` = 0 "
		    <<"  AND `map_object_type_id` = " <<EmperyCenter::MapObjectTypeIds::ID_CASTLE
		    <<"  AND `map_object_uuid` = " <<Poseidon::MySql::UuidFormatter(map_object_uuid.get());
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<EmperyCenter::MySql::Center_MapObject>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink->emplace_back(std::move(obj));
			}, "Center_MapObject", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		map_object_map->erase<0>(map_object_uuid);
		LOG_EMPERY_CENTER_DEBUG("Castle not found in database: map_object_uuid = ", map_object_uuid);
		return { };
	}

	auto map_object = reload_map_object_aux(std::move(sink->front()));
	map_object->pump_status();
	// map_object->recalculate_attributes(true);

	const auto elem = MapObjectElement(map_object);
	const auto result = map_object_map->insert(elem);
	if(!result.second){
		map_object_map->replace(result.first, elem);
	}

	LOG_EMPERY_CENTER_DEBUG("Successfully reloaded map object: map_object_uuid = ", map_object_uuid);
	return std::move(map_object);
}
void WorldMap::insert_map_object(const boost::shared_ptr<MapObject> &map_object){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		DEBUG_THROW(Exception, sslit("Map object map not loaded"));
	}

	const auto map_object_uuid = map_object->get_map_object_uuid();

	if(map_object->is_virtually_removed()){
		LOG_EMPERY_CENTER_WARNING("Map object has been marked as deleted: map_object_uuid = ", map_object_uuid);
		DEBUG_THROW(Exception, sslit("Map object has been marked as deleted"));
	}
	const auto new_coord = map_object->get_coord();

	LOG_EMPERY_CENTER_DEBUG("Inserting map object: map_object_uuid = ", map_object_uuid, ", new_coord = ", new_coord);
	const auto result = map_object_map->insert(MapObjectElement(map_object));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Map object already exists: map_object_uuid = ", map_object_uuid);
		DEBUG_THROW(Exception, sslit("Map object already exists"));
	}

	const auto owner_uuid = map_object->get_owner_uuid();
	const auto session = PlayerSessionMap::get(owner_uuid);
	if(session){
		try {
			synchronize_map_object_with_player(map_object, session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
	synchronize_with_all_players(map_object, new_coord, new_coord, session);
	synchronize_with_all_clusters(map_object, new_coord, new_coord);
	synchronize_with_controller(map_object, new_coord, new_coord);
}
void WorldMap::update_map_object(const boost::shared_ptr<MapObject> &map_object, bool throws_if_not_exists){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map object map not loaded"));
		}
		return;
	}

	const auto map_object_uuid = map_object->get_map_object_uuid();

	const auto it = map_object_map->find<0>(map_object_uuid);
	if(it == map_object_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Map object not found: map_object_uuid = ", map_object_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map object not found"));
		}
		return;
	}
	const auto old_coord = it->coord;
	const auto new_coord = map_object->get_coord();

	LOG_EMPERY_CENTER_TRACE("Updating map object: map_object_uuid = ", map_object_uuid, ", old_coord = ", old_coord, ", new_coord = ", new_coord);
	if(map_object->is_virtually_removed() || should_erase_map_object(map_object)){
		map_object_map->erase<0>(it);
	} else {
		map_object_map->replace<0>(it, MapObjectElement(map_object));
	}

	const auto owner_uuid = map_object->get_owner_uuid();
	const auto session = PlayerSessionMap::get(owner_uuid);
	if(session){
		try {
			synchronize_map_object_with_player(map_object, session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
	synchronize_with_all_players(map_object, old_coord, new_coord, session);
	synchronize_with_all_clusters(map_object, old_coord, new_coord);
	synchronize_with_controller(map_object, old_coord, new_coord);
}

void WorldMap::get_all_map_objects(std::vector<boost::shared_ptr<MapObject>> &ret){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return;
	}

	ret.reserve(ret.size() + map_object_map->size());
	for(auto it = map_object_map->begin<0>(); it != map_object_map->end<0>(); ++it){
		ret.emplace_back(it->map_object);
	}
}
void WorldMap::get_map_objects_by_owner(std::vector<boost::shared_ptr<MapObject>> &ret, AccountUuid owner_uuid){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return;
	}

	const auto range = map_object_map->equal_range<2>(owner_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->map_object);
	}
}
void WorldMap::forced_reload_map_objects_by_owner(AccountUuid owner_uuid){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return;
	}

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapObject>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_MapObject` WHERE `deleted` = 0 "
		    <<"  AND `owner_uuid` = " <<Poseidon::MySql::UuidFormatter(owner_uuid.get());
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<EmperyCenter::MySql::Center_MapObject>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink->emplace_back(std::move(obj));
			}, "Center_MapObject", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	for(const auto &obj : *sink){
		auto map_object = reload_map_object_aux(obj);
		const auto map_object_uuid = map_object->get_map_object_uuid();

		const auto elem = MapObjectElement(std::move(map_object));
		const auto result = map_object_map->insert(elem);
		if(!result.second){
			map_object_map->replace(result.first, elem);
		}

		LOG_EMPERY_CENTER_DEBUG("Successfully reloaded map object: map_object_uuid = ", map_object_uuid);
	}

	LOG_EMPERY_CENTER_DEBUG("Recalculating castle attributes...");
	std::deque<boost::shared_ptr<Castle>> castles_to_pump;
	const auto range = map_object_map->equal_range<2>(owner_uuid);
	for(auto it = range.first; it != range.second; ++it){
		auto castle = boost::dynamic_pointer_cast<Castle>(it->map_object);
		if(!castle){
			continue;
		}
		castles_to_pump.emplace_back(std::move(castle));
	}
	for(auto it = castles_to_pump.begin(); it != castles_to_pump.end(); ++it){
		const auto &castle = *it;
		try {
			castle->pump_status();
			// castle->recalculate_attributes(true);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}
}
void WorldMap::get_map_objects_by_parent_object(std::vector<boost::shared_ptr<MapObject>> &ret, MapObjectUuid parent_object_uuid){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return;
	}

	const auto range = map_object_map->equal_range<3>(parent_object_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->map_object);
	}
}
void WorldMap::forced_reload_map_objects_by_parent_object(MapObjectUuid parent_object_uuid){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return;
	}

	const auto castle = boost::dynamic_pointer_cast<Castle>(get_map_object(parent_object_uuid));
	if(!castle){
		LOG_EMPERY_CENTER_WARNING("Castle not found: parent_object_uuid = ", parent_object_uuid);
		return;
	}

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapObject>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_MapObject` WHERE `deleted` = 0 "
		    <<"  AND `map_object_type_id` != " <<EmperyCenter::MapObjectTypeIds::ID_CASTLE
		    <<"  AND `parent_object_uuid` = " <<Poseidon::MySql::UuidFormatter(parent_object_uuid.get());
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<EmperyCenter::MySql::Center_MapObject>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink->emplace_back(std::move(obj));
			}, "Center_MapObject", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	for(const auto &obj : *sink){
		auto map_object = reload_map_object_aux(obj);
		const auto map_object_uuid = map_object->get_map_object_uuid();

		const auto elem = MapObjectElement(std::move(map_object));
		const auto result = map_object_map->insert(elem);
		if(!result.second){
			map_object_map->replace(result.first, elem);
		}

		LOG_EMPERY_CENTER_DEBUG("Successfully reloaded map object: map_object_uuid = ", map_object_uuid);
	}

	LOG_EMPERY_CENTER_DEBUG("Recalculating castle attributes...");
	castle->pump_status();
	// castle->recalculate_attributes(true);
}
boost::shared_ptr<MapObject> WorldMap::get_map_object_by_garrisoning_object(MapObjectUuid garrisoning_object_uuid){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return { };
	}

	const auto it = map_object_map->find<4>(garrisoning_object_uuid);
	if(it == map_object_map->end<4>()){
		return { };
	}
	return it->map_object;
}
void WorldMap::get_map_objects_by_rectangle(std::vector<boost::shared_ptr<MapObject>> &ret, Rectangle rectangle){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return;
	}

	auto x = rectangle.left();
	while(x < rectangle.right()){
		auto it = map_object_map->lower_bound<1>(Coord(x, rectangle.bottom()));
		for(;;){
			if(it == map_object_map->end<1>()){
				goto _exit_while;
			}
			if(it->coord.x() != x){
				x = it->coord.x();
				break;
			}
			if(it->coord.y() >= rectangle.top()){
				++x;
				break;
			}
			ret.emplace_back(it->map_object);
			++it;
		}
	}
_exit_while:
	;
}

boost::shared_ptr<Castle> WorldMap::get_primary_castle(AccountUuid owner_uuid){
	PROFILE_ME;

	boost::shared_ptr<Castle> primary_castle;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		DEBUG_THROW(Exception, sslit("Map object map not loaded"));
	}

	const auto range = map_object_map->equal_range<2>(owner_uuid);
	for(auto it = range.first; it != range.second; ++it){
		const auto &map_object = it->map_object;
		if(map_object->get_map_object_type_id() != MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		const auto castle = boost::dynamic_pointer_cast<Castle>(map_object);
		if(!castle){
			continue;
		}
		if(primary_castle && (primary_castle->get_map_object_uuid() <= castle->get_map_object_uuid())){
			continue;
		}
		primary_castle = castle;
	}

	return primary_castle;
}
boost::shared_ptr<Castle> WorldMap::require_primary_castle(AccountUuid owner_uuid){
	PROFILE_ME;

	auto ret = get_primary_castle(owner_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Player has no castle? owner_uuid = ", owner_uuid);
		DEBUG_THROW(Exception, sslit("Player has no castle"));
	}
	return ret;
}

boost::shared_ptr<StrategicResource> WorldMap::get_strategic_resource(Coord coord){
	PROFILE_ME;

	const auto strategic_resource_map = g_strategic_resource_map.lock();
	if(!strategic_resource_map){
		LOG_EMPERY_CENTER_WARNING("Strategic resource map not loaded.");
		return { };
	}

	const auto it = strategic_resource_map->find<0>(coord);
	if(it == strategic_resource_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Strategic resource not found: coord = ", coord);
		return { };
	}
	return it->strategic_resource;
}
boost::shared_ptr<StrategicResource> WorldMap::require_strategic_resource(Coord coord){
	PROFILE_ME;

	auto ret = get_strategic_resource(coord);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Strategic resource not found: coord = ", coord);
		DEBUG_THROW(Exception, sslit("Strategic resource not found"));
	}
	return ret;
}
void WorldMap::insert_strategic_resource(const boost::shared_ptr<StrategicResource> &strategic_resource){
	PROFILE_ME;

	const auto strategic_resource_map = g_strategic_resource_map.lock();
	if(!strategic_resource_map){
		LOG_EMPERY_CENTER_WARNING("Strategic resource map not loaded.");
		DEBUG_THROW(Exception, sslit("Strategic resource map not loaded"));
	}

	const auto coord = strategic_resource->get_coord();

	LOG_EMPERY_CENTER_TRACE("Inserting strategic resource: coord = ", coord, ", resource_id = ", strategic_resource->get_resource_id());
	const auto result = strategic_resource_map->insert(StrategicResourceElement(strategic_resource));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Strategic resource already exists: coord = ", coord);
		DEBUG_THROW(Exception, sslit("Strategic resource already exists"));
	}

	synchronize_with_all_players(strategic_resource, coord, coord, { });
}
void WorldMap::update_strategic_resource(const boost::shared_ptr<StrategicResource> &strategic_resource, bool throws_if_not_exists){
	PROFILE_ME;

	const auto strategic_resource_map = g_strategic_resource_map.lock();
	if(!strategic_resource_map){
		LOG_EMPERY_CENTER_WARNING("Strategic resource map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Strategic resource map not loaded"));
		}
		return;
	}

	const auto coord = strategic_resource->get_coord();

	const auto it = strategic_resource_map->find<0>(coord);
	if(it == strategic_resource_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Strategic resource not found: coord = ", coord);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("StrategicResource not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_TRACE("Updating strategic resource: coord = ", coord, ", resource_id = ", strategic_resource->get_resource_id());
	if(strategic_resource->is_virtually_removed()){
		strategic_resource_map->erase<0>(it);
	} else {
		strategic_resource_map->replace<0>(it, StrategicResourceElement(strategic_resource));
	}

	synchronize_with_all_players(strategic_resource, coord, coord, { });
}

void WorldMap::get_strategic_resources_by_rectangle(std::vector<boost::shared_ptr<StrategicResource>> &ret, Rectangle rectangle){
	PROFILE_ME;

	const auto strategic_resource_map = g_strategic_resource_map.lock();
	if(!strategic_resource_map){
		LOG_EMPERY_CENTER_WARNING("Strategic resource map not loaded.");
		return;
	}

	auto x = rectangle.left();
	while(x < rectangle.right()){
		auto it = strategic_resource_map->lower_bound<0>(Coord(x, rectangle.bottom()));
		for(;;){
			if(it == strategic_resource_map->end<0>()){
				goto _exit_while;
			}
			if(it->coord.x() != x){
				x = it->coord.x();
				break;
			}
			if(it->coord.y() >= rectangle.top()){
				++x;
				break;
			}
			ret.emplace_back(it->strategic_resource);
			++it;
		}
	}
_exit_while:
	;
}

boost::shared_ptr<MapEventBlock> WorldMap::get_map_event_block(Coord coord){
	PROFILE_ME;

	const auto map_event_block_map = g_map_event_block_map.lock();
	if(!map_event_block_map){
		LOG_EMPERY_CENTER_WARNING("Map event block map not loaded.");
		return { };
	}

	const auto block_coord = get_event_block_coord_from_world_coord(coord);
	const auto it = map_event_block_map->find<0>(block_coord);
	if(it == map_event_block_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Map event block not found: coord = ", coord, ", block_coord = ", block_coord);
		return { };
	}
	return it->map_event_block;
}
boost::shared_ptr<MapEventBlock> WorldMap::require_map_event_block(Coord coord){
	PROFILE_ME;

	auto ret = get_map_event_block(coord);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Map event block not found: coord = ", coord);
		DEBUG_THROW(Exception, sslit("Map event block map not found"));
	}
	return ret;
}
void WorldMap::insert_map_event_block(const boost::shared_ptr<MapEventBlock> &map_event_block){
	PROFILE_ME;

	const auto map_event_block_map = g_map_event_block_map.lock();
	if(!map_event_block_map){
		LOG_EMPERY_CENTER_WARNING("Map event block map not loaded.");
		DEBUG_THROW(Exception, sslit("Map event block map not loaded"));
	}

	const auto block_coord = map_event_block->get_block_coord();

	LOG_EMPERY_CENTER_TRACE("Inserting map event block: block_coord = ", block_coord);
	const auto result = map_event_block_map->insert(MapEventBlockElement(map_event_block));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Map event block already exists: block_coord = ", block_coord);
		DEBUG_THROW(Exception, sslit("Map event block already exists"));
	}
}
void WorldMap::update_map_event_block(const boost::shared_ptr<MapEventBlock> &map_event_block, bool throws_if_not_exists){
	PROFILE_ME;

	const auto map_event_block_map = g_map_event_block_map.lock();
	if(!map_event_block_map){
		LOG_EMPERY_CENTER_WARNING("Map event block map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map event block map not loaded"));
		}
		return;
	}

	const auto block_coord = map_event_block->get_block_coord();

	const auto it = map_event_block_map->find<0>(block_coord);
	if(it == map_event_block_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Map event block map not found: block_coord = ", block_coord);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map event block map not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating map event block: block_coord = ", block_coord);
	map_event_block_map->replace<0>(it, MapEventBlockElement(map_event_block));
}

boost::shared_ptr<ResourceCrate> WorldMap::get_resource_crate(ResourceCrateUuid resource_crate_uuid){
	PROFILE_ME;

	const auto resource_crate_map = g_resource_crate_map.lock();
	if(!resource_crate_map){
		LOG_EMPERY_CENTER_WARNING("Resource crate map not loaded.");
		return { };
	}

	const auto it = resource_crate_map->find<0>(resource_crate_uuid);
	if(it == resource_crate_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Resource crate not found: resource_crate_uuid = ", resource_crate_uuid);
		return { };
	}
	return it->resource_crate;
}
boost::shared_ptr<ResourceCrate> WorldMap::require_resource_crate(ResourceCrateUuid resource_crate_uuid){
	PROFILE_ME;

	auto ret = get_resource_crate(resource_crate_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Resource crate not found: resource_crate_uuid = ", resource_crate_uuid);
		DEBUG_THROW(Exception, sslit("Resource crate not found"));
	}
	return ret;
}
void WorldMap::insert_resource_crate(const boost::shared_ptr<ResourceCrate> &resource_crate){
	PROFILE_ME;

	const auto resource_crate_map = g_resource_crate_map.lock();
	if(!resource_crate_map){
		LOG_EMPERY_CENTER_WARNING("Resource crate map not loaded.");
		DEBUG_THROW(Exception, sslit("Resource crate map not loaded"));
	}

	const auto resource_crate_uuid = resource_crate->get_resource_crate_uuid();
	const auto coord = resource_crate->get_coord();

	LOG_EMPERY_CENTER_TRACE("Inserting resource crate: resource_crate_uuid = ", resource_crate_uuid);
	const auto result = resource_crate_map->insert(ResourceCrateElement(resource_crate));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Resource crate already exists: resource_crate_uuid = ", resource_crate_uuid);
		DEBUG_THROW(Exception, sslit("Resource crate already exists"));
	}

	synchronize_with_all_players(resource_crate, coord, coord, { });
	synchronize_with_all_clusters(resource_crate, coord, coord);
}
void WorldMap::update_resource_crate(const boost::shared_ptr<ResourceCrate> &resource_crate, bool throws_if_not_exists){
	PROFILE_ME;

	const auto resource_crate_map = g_resource_crate_map.lock();
	if(!resource_crate_map){
		LOG_EMPERY_CENTER_WARNING("Resource crate map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Resource crate map not loaded"));
		}
		return;
	}

	const auto resource_crate_uuid = resource_crate->get_resource_crate_uuid();
	const auto coord = resource_crate->get_coord();

	const auto it = resource_crate_map->find<0>(resource_crate_uuid);
	if(it == resource_crate_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Resource crate not found: resource_crate_uuid = ", resource_crate_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("ResourceCrate not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_TRACE("Updating resource crate: resource_crate_uuid = ", resource_crate_uuid);
	if(resource_crate->is_virtually_removed()){
		resource_crate_map->erase<0>(it);
	} else {
		resource_crate_map->replace<0>(it, ResourceCrateElement(resource_crate));
	}

	synchronize_with_all_players(resource_crate, coord, coord, { });
	synchronize_with_all_clusters(resource_crate, coord, coord);
}

void WorldMap::get_resource_crates_by_rectangle(std::vector<boost::shared_ptr<ResourceCrate>> &ret, Rectangle rectangle){
	PROFILE_ME;

	const auto resource_crate_map = g_resource_crate_map.lock();
	if(!resource_crate_map){
		LOG_EMPERY_CENTER_WARNING("Resource crate map not loaded.");
		return;
	}

	auto x = rectangle.left();
	while(x < rectangle.right()){
		auto it = resource_crate_map->lower_bound<1>(Coord(x, rectangle.bottom()));
		for(;;){
			if(it == resource_crate_map->end<1>()){
				goto _exit_while;
			}
			if(it->coord.x() != x){
				x = it->coord.x();
				break;
			}
			if(it->coord.y() >= rectangle.top()){
				++x;
				break;
			}
			ret.emplace_back(it->resource_crate);
			++it;
		}
	}
_exit_while:
	;
}

void WorldMap::get_players_viewing_rectangle(std::vector<boost::shared_ptr<PlayerSession>> &ret, Rectangle rectangle){
	PROFILE_ME;

	const auto player_view_map = g_player_view_map.lock();
	if(!player_view_map){
		LOG_EMPERY_CENTER_WARNING("Player view map not initialized.");
		return;
	}

	boost::container::flat_set<boost::shared_ptr<PlayerSession>> temp;

	const auto sector_bottom_left = get_sector_coord_from_world_coord(Coord(rectangle.left(), rectangle.bottom()));
	const auto sector_upper_right = get_sector_coord_from_world_coord(Coord(rectangle.right() - 1, rectangle.top() - 1));
	for(auto sector_x = sector_bottom_left.x(); sector_x <= sector_upper_right.x(); sector_x += SECTOR_SIDE_LENGTH){
		for(auto sector_y = sector_bottom_left.y(); sector_y <= sector_upper_right.y(); sector_y += SECTOR_SIDE_LENGTH){
			const auto sector_coord = Coord(sector_x, sector_y);
			const auto range = player_view_map->equal_range<1>(sector_coord);
			temp.reserve(temp.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
			for(auto next = range.first, it = next; (next != range.second) && (++next, true); it = next){
				auto session = it->session.lock();
				if(!session){
					player_view_map->erase<1>(it);
					continue;
				}
				const auto view = session->get_view();
				if((view.left() < rectangle.right()) && (rectangle.left() < view.right()) &&
					(view.bottom() < rectangle.top()) && (rectangle.bottom() < view.top()))
				{
					temp.emplace(std::move(session));
				}
			}
		}
	}

	ret.reserve(ret.size() + temp.size());
	for(auto it = temp.begin(); it != temp.end(); ++it){
		ret.emplace_back(*it);
	}
}
void WorldMap::update_player_view(const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	const auto player_view_map = g_player_view_map.lock();
	if(!player_view_map){
		LOG_EMPERY_CENTER_WARNING("Player view map not initialized.");
		DEBUG_THROW(Exception, sslit("Player view map not initialized"));
	}

	const auto view = session->get_view();

	player_view_map->erase<0>(session);

	if((view.width() == 0) || (view.height() == 0)){
		return;
	}

	const auto sector_bottom_left = get_sector_coord_from_world_coord(Coord(view.left(), view.bottom()));
	const auto sector_upper_right = get_sector_coord_from_world_coord(Coord(view.right() - 1, view.top() - 1));
	LOG_EMPERY_CENTER_DEBUG("Set player view: view = ", view,
		", sector_bottom_left = ", sector_bottom_left, ", sector_upper_right = ", sector_upper_right);
	try {
		for(auto sector_x = sector_bottom_left.x(); sector_x <= sector_upper_right.x(); sector_x += SECTOR_SIDE_LENGTH){
			for(auto sector_y = sector_bottom_left.y(); sector_y <= sector_upper_right.y(); sector_y += SECTOR_SIDE_LENGTH){
				player_view_map->insert(PlayerViewElement(session, Coord(sector_x, sector_y)));
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		player_view_map->erase<0>(session);
		throw;
	}
}

void WorldMap::synchronize_player_view(const boost::shared_ptr<PlayerSession> &session, Rectangle view) noexcept
try {
	PROFILE_ME;

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	get_map_cells_by_rectangle(map_cells, view);
	for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
		const auto &map_cell = *it;
		if(map_cell->is_virtually_removed()){
			continue;
		}
		synchronize_map_cell_with_player(map_cell, session);
	}

	std::vector<boost::shared_ptr<MapObject>> map_objects;
	get_map_objects_by_rectangle(map_objects, view);
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = *it;
		if(map_object->is_virtually_removed()){
			continue;
		}
		if(map_object->is_garrisoned()){
			continue;
		}
		synchronize_map_object_with_player(map_object, session);
	}

	std::vector<boost::shared_ptr<StrategicResource>> strategic_resources;
	get_strategic_resources_by_rectangle(strategic_resources, view);
	for(auto it = strategic_resources.begin(); it != strategic_resources.end(); ++it){
		const auto &strategic_resource = *it;
		if(strategic_resource->is_virtually_removed()){
			continue;
		}
		synchronize_strategic_resource_with_player(strategic_resource, session);
	}

	std::vector<boost::shared_ptr<ResourceCrate>> resource_crates;
	get_resource_crates_by_rectangle(resource_crates, view);
	for(auto it = resource_crates.begin(); it != resource_crates.end(); ++it){
		const auto &resource_crate = *it;
		if(resource_crate->is_virtually_removed()){
			continue;
		}
		synchronize_resource_crate_with_player(resource_crate, session);
	}
} catch(std::exception &e){
	LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	session->shutdown(e.what());
}

Rectangle WorldMap::get_cluster_scope(Coord coord){
	PROFILE_ME;

	return Rectangle(get_cluster_coord_from_world_coord(coord), MAP_WIDTH, MAP_HEIGHT);
}

boost::shared_ptr<ClusterSession> WorldMap::get_cluster(Coord coord){
	PROFILE_ME;

	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CENTER_WARNING("Cluster map not loaded.");
		return { };
	}

	const auto cluster_coord = get_cluster_coord_from_world_coord(coord);
	const auto it = cluster_map->find<0>(cluster_coord);
	if(it == cluster_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Cluster not found: coord = ", coord, ", cluster_coord = ", cluster_coord);
		return { };
	}
	auto cluster = it->cluster.lock();
	if(!cluster){
		LOG_EMPERY_CENTER_DEBUG("Cluster expired: coord = ", coord, ", cluster_coord = ", cluster_coord);
		cluster_map->erase<0>(it);
		return { };
	}
	return std::move(cluster);
}
void WorldMap::get_all_clusters(std::vector<std::pair<Coord, boost::shared_ptr<ClusterSession>>> &ret){
	PROFILE_ME;

	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CENTER_WARNING("Cluster map not loaded.");
		return;
	}

	ret.reserve(ret.size() + cluster_map->size());
	for(auto it = cluster_map->begin(); it != cluster_map->end(); ++it){
		auto cluster = it->cluster.lock();
		if(!cluster){
			continue;
		}
		ret.emplace_back(it->cluster_coord, std::move(cluster));
	}
}
void WorldMap::set_cluster(const boost::shared_ptr<ClusterSession> &cluster, Coord coord){
	PROFILE_ME;

	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CENTER_WARNING("Cluster map not loaded.");
		DEBUG_THROW(Exception, sslit("Cluster map not loaded"));
	}

	const auto cit = cluster_map->find<1>(cluster);
	if(cit != cluster_map->end<1>()){
		LOG_EMPERY_CENTER_WARNING("Cluster already registered: old_cluster_coord = ", cit->cluster_coord);
		DEBUG_THROW(Exception, sslit("Cluster already registered"));
	}

	const auto scope = get_cluster_scope(coord);
	const auto cluster_coord = scope.bottom_left();
	LOG_EMPERY_CENTER_INFO("Setting up cluster server: cluster_coord = ", cluster_coord);
	const auto result = cluster_map->insert(ClusterElement(cluster_coord, cluster));
	if(!result.second){
		const auto old_cluster = result.first->cluster.lock();
		if(old_cluster && (old_cluster != cluster)){
			LOG_EMPERY_CENTER_WARNING("Killing old cluster server: cluster_coord = ", cluster_coord);
			old_cluster->shutdown(Msg::KILL_CLUSTER_SERVER_CONFLICT, "");
		}
		cluster_map->replace(result.first, ClusterElement(cluster_coord, cluster));
	}
}
void WorldMap::forced_reload_cluster(Coord coord){
	PROFILE_ME;

	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CENTER_WARNING("Cluster map not loaded.");
		DEBUG_THROW(Exception, sslit("Cluster map not loaded"));
	}

	const auto scope = get_cluster_scope(coord);
	const auto cluster_coord = scope.bottom_left();

	std::size_t concurrency_counter = 0;

#define CONCURRENT_LOAD_BEGIN	\
	{	\
		Poseidon::enqueue_async_job(	\
			[=]() mutable {	\
				PROFILE_ME;	\
				try
#define CONCURRENT_LOAD_END	\
				catch(std::exception &e_){	\
					LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e_.what());	\
				}	\
			});	\
		if(++concurrency_counter >= 100){	\
			LOG_EMPERY_CENTER_DEBUG("Too many async requests have been enqueued. Yielding...");	\
			const auto promise_ = Poseidon::MySqlDaemon::enqueue_for_waiting_for_all_async_operations();	\
			Poseidon::JobDispatcher::yield(promise_, true);	\
			concurrency_counter = 0;	\
		}	\
	}

	const auto map_cell_map = g_map_cell_map.lock();
	if(map_cell_map){
		LOG_EMPERY_CENTER_INFO("Loading map cells: scope = ", scope);

		const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapCell>>>();
		{
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_MapCell` WHERE "
				<<scope.left() <<" <= `x` AND `x` < " <<scope.right() <<"  AND " <<scope.bottom() <<" <= `y` AND `y` < " <<scope.top();
			const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
				[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
					auto obj = boost::make_shared<EmperyCenter::MySql::Center_MapCell>();
					obj->fetch(conn);
					obj->enable_auto_saving();
					sink->emplace_back(std::move(obj));
				}, "Center_MapCell", oss.str());
			Poseidon::JobDispatcher::yield(promise, true);
		}
		for(const auto &obj : *sink){
			CONCURRENT_LOAD_BEGIN {
				auto map_cell = reload_map_cell_aux(obj);
				const auto elem = MapCellElement(std::move(map_cell));
				const auto result = map_cell_map->insert(elem);
				if(!result.second){
					map_cell_map->replace(result.first, elem);
				}
			} CONCURRENT_LOAD_END;
		}
	}

	const auto map_object_map = g_map_object_map.lock();
	if(map_object_map){
		LOG_EMPERY_CENTER_INFO("Loading map objects: scope = ", scope);

		const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapObject>>>();
		{
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_MapObject` WHERE `deleted` = 0 AND "
				<<scope.left() <<" <= `x` AND `x` < " <<scope.right() <<"  AND " <<scope.bottom() <<" <= `y` AND `y` < " <<scope.top();
			const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
				[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
					auto obj = boost::make_shared<EmperyCenter::MySql::Center_MapObject>();
					obj->fetch(conn);
					obj->enable_auto_saving();
					sink->emplace_back(std::move(obj));
				}, "Center_MapObject", oss.str());
			Poseidon::JobDispatcher::yield(promise, true);
		}
		for(const auto &obj : *sink){
			CONCURRENT_LOAD_BEGIN {
				auto map_object = reload_map_object_aux(obj);
				const auto elem = MapObjectElement(std::move(map_object));
				const auto result = map_object_map->insert(elem);
				if(!result.second){
					map_object_map->replace(result.first, elem);
				}
			} CONCURRENT_LOAD_END;
		}
	}

	const auto strategic_resource_map = g_strategic_resource_map.lock();
	if(strategic_resource_map){
		LOG_EMPERY_CENTER_INFO("Loading strategic resources: scope = ", scope);

		const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_StrategicResource>>>();
		{
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_StrategicResource` WHERE "
				<<scope.left() <<" <= `x` AND `x` < " <<scope.right() <<"  AND " <<scope.bottom() <<" <= `y` AND `y` < " <<scope.top();
			const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
				[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
					auto obj = boost::make_shared<EmperyCenter::MySql::Center_StrategicResource>();
					obj->fetch(conn);
					obj->enable_auto_saving();
					sink->emplace_back(std::move(obj));
				}, "Center_StrategicResource", oss.str());
			Poseidon::JobDispatcher::yield(promise, true);
		}
		for(const auto &obj : *sink){
			CONCURRENT_LOAD_BEGIN {
				auto strategic_resource = boost::make_shared<StrategicResource>(obj);
				const auto elem = StrategicResourceElement(std::move(strategic_resource));
				const auto result = strategic_resource_map->insert(elem);
				if(!result.second){
					strategic_resource_map->replace(result.first, elem);
				}
			} CONCURRENT_LOAD_END;
		}
	}

	const auto map_event_block_map = g_map_event_block_map.lock();
	if(map_event_block_map){
		LOG_EMPERY_CENTER_INFO("Loading map event block: scope = ", scope);

		const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_MapEventBlock>>>();
		{
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_MapEventBlock` WHERE "
				<<scope.left() <<" <= `block_x` AND `block_x` < " <<scope.right() <<"  AND "
				<<scope.bottom() <<" <= `block_y` AND `block_y` < " <<scope.top();
			const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
				[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
					auto obj = boost::make_shared<EmperyCenter::MySql::Center_MapEventBlock>();
					obj->fetch(conn);
					obj->enable_auto_saving();
					sink->emplace_back(std::move(obj));
				}, "Center_MapEventBlock", oss.str());
			Poseidon::JobDispatcher::yield(promise, true);
		}
		for(const auto &obj : *sink){
			CONCURRENT_LOAD_BEGIN {
				auto map_event_block = reload_map_event_block_aux(obj);
				const auto elem = MapEventBlockElement(std::move(map_event_block));
				const auto result = map_event_block_map->insert(elem);
				if(!result.second){
					map_event_block_map->replace(result.first, elem);
				}
			} CONCURRENT_LOAD_END;
		}

		boost::container::flat_set<Coord> map_event_block_new;
		map_event_block_new.reserve((MAP_WIDTH / EVENT_BLOCK_WIDTH) * (MAP_HEIGHT / EVENT_BLOCK_HEIGHT));
		for(unsigned map_y = 0; map_y < MAP_HEIGHT; map_y += EVENT_BLOCK_HEIGHT){
			for(unsigned map_x = 0; map_x < MAP_WIDTH; map_x += EVENT_BLOCK_WIDTH){
				map_event_block_new.insert(Coord(cluster_coord.x() + map_x, cluster_coord.y() + map_y));
			}
		}
		LOG_EMPERY_CENTER_DEBUG("?!> Number of initial map event blocks: ", map_event_block_new.size());
		for(const auto &obj : *sink){
			const auto block_coord = Coord(obj->get_block_x(), obj->get_block_y());
			map_event_block_new.erase(block_coord);
		}
		LOG_EMPERY_CENTER_DEBUG("?!> Number of map event blocks to create: ", map_event_block_new.size());
		for(const auto &coord : map_event_block_new){
			CONCURRENT_LOAD_BEGIN {
				auto map_event_block = boost::make_shared<MapEventBlock>(coord);
				const auto elem = MapEventBlockElement(std::move(map_event_block));
				const auto result = map_event_block_map->insert(elem);
				if(!result.second){
					map_event_block_map->replace(result.first, elem);
				}
			} CONCURRENT_LOAD_END;
		}
	}

	const auto resource_crate_map = g_resource_crate_map.lock();
	if(resource_crate_map){
		LOG_EMPERY_CENTER_INFO("Loading resource crates: scope = ", scope);

		const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_ResourceCrate>>>();
		{
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_ResourceCrate` WHERE `amount_remaining` > 0 AND "
				<<scope.left() <<" <= `x` AND `x` < " <<scope.right() <<"  AND " <<scope.bottom() <<" <= `y` AND `y` < " <<scope.top();
			const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
				[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
					auto obj = boost::make_shared<EmperyCenter::MySql::Center_ResourceCrate>();
					obj->fetch(conn);
					obj->enable_auto_saving();
					sink->emplace_back(std::move(obj));
				}, "Center_ResourceCrate", oss.str());
			Poseidon::JobDispatcher::yield(promise, true);
		}
		for(const auto &obj : *sink){
			CONCURRENT_LOAD_BEGIN {
				auto resource_crate = boost::make_shared<ResourceCrate>(obj);
				const auto elem = ResourceCrateElement(std::move(resource_crate));
				const auto result = resource_crate_map->insert(elem);
				if(!result.second){
					resource_crate_map->replace(result.first, elem);
				}
			} CONCURRENT_LOAD_END;
		}
	}
}
void WorldMap::synchronize_cluster(const boost::shared_ptr<ClusterSession> &cluster, Rectangle view) noexcept
try {
	PROFILE_ME;

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	get_map_cells_by_rectangle(map_cells, view);
	for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
		const auto &map_cell = *it;
		if(map_cell->is_virtually_removed()){
			continue;
		}
		synchronize_map_cell_with_cluster(map_cell, cluster);
	}

	std::vector<boost::shared_ptr<MapObject>> map_objects;
	get_map_objects_by_rectangle(map_objects, view);
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = *it;
		if(map_object->is_virtually_removed()){
			continue;
		}
		synchronize_map_object_with_cluster(map_object, cluster);
	}

	std::vector<boost::shared_ptr<ResourceCrate>> resource_crates;
	get_resource_crates_by_rectangle(resource_crates, view);
	for(auto it = resource_crates.begin(); it != resource_crates.end(); ++it){
		const auto &resource_crate = *it;
		if(resource_crate->is_virtually_removed()){
			continue;
		}
		synchronize_resource_crate_with_cluster(resource_crate, cluster);
	}
} catch(std::exception &e){
	LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	cluster->shutdown(e.what());
}

boost::shared_ptr<Castle> WorldMap::place_castle_random_restricted(
	const boost::function<boost::shared_ptr<Castle> (Coord)> &factory, Coord coord_hint)
{
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return { };
	}

	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CENTER_WARNING("Cluster map is not loaded.");
		return { };
	}

	const auto cluster_coord = get_cluster_coord_from_world_coord(coord_hint);
	const auto cit = cluster_map->find<0>(cluster_coord);
	if(cit == cluster_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Cluster not found: coord_hint = ", coord_hint, ", cluster_coord = ", cluster_coord);
		return { };
	}
	auto &cached_start_points = cit->cached_start_points;

	const auto get_start_point = [&]{
		boost::shared_ptr<Castle> castle;
		for(;;){
			const auto point_count = cached_start_points.size();
			if(point_count == 0){
				break;
			}
			const auto index = static_cast<std::size_t>(Poseidon::rand64() % point_count);
			const auto coord = cached_start_points.at(index);
			LOG_EMPERY_CENTER_DEBUG("Try placing castle: coord = ", coord);

			const auto result = can_deploy_castle_at(coord, { });
			if(result.first == 0){
				castle = factory(coord);
				if(!castle){
					DEBUG_THROW(Exception, sslit("No castle allocated"));
				}
				castle->pump_status();
				castle->check_init_buildings();
				castle->check_init_resources();
				castle->recalculate_attributes(false);

				LOG_EMPERY_CENTER_DEBUG("Creating castle: coord = ", coord);
				map_object_map->insert(MapObjectElement(castle));
				update_map_object(castle, false);
			}
			cached_start_points.erase(cached_start_points.begin() + static_cast<std::ptrdiff_t>(index));

			if(castle){
				LOG_EMPERY_CENTER_DEBUG("Castle placed successfully: map_object_uuid = ", castle->get_map_object_uuid(),
					", owner_uuid = ", castle->get_owner_uuid(), ", coord = ", coord);
				break;
			}
		}
		return std::move(castle);
	};

	auto castle = get_start_point();
	if(!castle){
		LOG_EMPERY_CENTER_DEBUG("We have run out of start points: cluster_coord = ", cluster_coord);
		std::vector<boost::shared_ptr<const Data::MapStartPoint>> start_points;
		Data::MapStartPoint::get_all(start_points);
		cached_start_points.clear();
		cached_start_points.reserve(start_points.size());
		for(auto it = start_points.begin(); it != start_points.end(); ++it){
			const auto &start_point_data = *it;
			cached_start_points.emplace_back(cluster_coord.x() + start_point_data->map_coord.first,
			                                 cluster_coord.y() + start_point_data->map_coord.second);
		}
		LOG_EMPERY_CENTER_DEBUG("Retrying: cluster_coord = ", cluster_coord, ", start_point_count = ", cached_start_points.size());
		castle = get_start_point();
	}
	return castle;
}
boost::shared_ptr<Castle> WorldMap::place_castle_random(
	const boost::function<boost::shared_ptr<Castle> (Coord)> &factory)
{
	PROFILE_ME;

	std::vector<std::pair<Coord, boost::shared_ptr<ClusterSession>>> clusters;
	get_all_clusters(clusters);
	if(clusters.empty()){
		LOG_EMPERY_CENTER_WARNING("No clusters available");
		return { };
	}

	auto it = clusters.end();
	{
		std::vector<boost::shared_ptr<MapObject>> map_objects;

		const auto count_castles_in_clusters = [&](Coord coord_hint){
			std::size_t castle_count = 0;
			map_objects.clear();
			WorldMap::get_map_objects_by_rectangle(map_objects, WorldMap::get_cluster_scope(coord_hint));
			for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
				const auto &map_object = *it;
				const auto map_object_type_id = map_object->get_map_object_type_id();
				if(map_object_type_id != MapObjectTypeIds::ID_CASTLE){
					continue;
				}
				++castle_count;
			}
			return castle_count;
		};

		const auto old_limit = GlobalStatus::cast<std::uint64_t>(GlobalStatus::SLOT_INIT_SERVER_LIMIT);
		if(old_limit != 0){
			const auto cluster_x = GlobalStatus::cast<std::int64_t>(GlobalStatus::SLOT_INIT_SERVER_X);
			const auto cluster_y = GlobalStatus::cast<std::int64_t>(GlobalStatus::SLOT_INIT_SERVER_Y);
			const auto coord_hint = Coord(cluster_x, cluster_y);
			LOG_EMPERY_CENTER_DEBUG("Testing cluster: coord_hint = ", coord_hint);
			it = std::find_if(clusters.begin(), clusters.end(), [&](decltype(clusters.front()) &pair){ return pair.first == coord_hint; });
			if(it != clusters.end()){
				const auto castle_count = count_castles_in_clusters(coord_hint);
				LOG_EMPERY_CENTER_DEBUG("Number of castles on cluster: coord_hint = ", coord_hint, ", castle_count = ", castle_count);
				if(castle_count < old_limit){
					LOG_EMPERY_CENTER_DEBUG("Max number of castles exceeded: castle_count = ", castle_count, ", old_limit = ", old_limit);
					goto _use_hint;
				}
			}
		}
		LOG_EMPERY_CENTER_INFO("Reselecting init cluster server...");

		boost::container::flat_multimap<std::size_t, Coord> clusters_by_castle_count;
		clusters_by_castle_count.reserve(clusters.size());
		for(auto cit = clusters.begin(); cit != clusters.end(); ++cit){
			const auto cluster_coord = cit->first;
			const auto castle_count = count_castles_in_clusters(cluster_coord);
			LOG_EMPERY_CENTER_INFO("Number of castles on cluster: cluster_coord = ", cluster_coord, ", castle_count = ", castle_count);
			clusters_by_castle_count.emplace(castle_count, cluster_coord);
		}
		const auto limit_max = get_config<std::uint64_t>("cluster_map_castle_limit_max", 700);
		clusters_by_castle_count.erase(clusters_by_castle_count.lower_bound(limit_max), clusters_by_castle_count.end());

		if(clusters_by_castle_count.empty()){
			LOG_EMPERY_CENTER_WARNING("No clusters available");
			return { };
		}
		const auto front_it = clusters_by_castle_count.begin();

		const auto limit_init = get_config<std::uint64_t>("cluster_map_castle_limit_init", 300);
		auto new_limit = limit_init;
		if(front_it->first >= limit_init){
			const auto limit_increment = get_config<std::uint64_t>("cluster_map_castle_limit_increment", 200);
			new_limit = saturated_add(new_limit, saturated_mul((front_it->first - limit_init) / limit_increment + 1, limit_increment));
		}
		LOG_EMPERY_CENTER_DEBUG("Resetting castle limit: old_limit = ", old_limit, ", new_limit = ", new_limit);

		auto limit_str = boost::lexical_cast<std::string>(new_limit);
		auto x_str     = boost::lexical_cast<std::string>(front_it->second.x());
		auto y_str     = boost::lexical_cast<std::string>(front_it->second.y());
		GlobalStatus::set(GlobalStatus::SLOT_INIT_SERVER_LIMIT, std::move(limit_str));
		GlobalStatus::set(GlobalStatus::SLOT_INIT_SERVER_X,     std::move(x_str));
		GlobalStatus::set(GlobalStatus::SLOT_INIT_SERVER_Y,     std::move(y_str));

		const auto cluster_coord = front_it->second;
		LOG_EMPERY_CENTER_DEBUG("Selected cluster server: cluster_coord = ", cluster_coord);
		it = std::find_if(clusters.begin(), clusters.end(), [&](decltype(clusters.front()) &pair){ return pair.first == cluster_coord; });
		if(it != clusters.end()){
			goto _use_hint;
		}
	}
	LOG_EMPERY_CENTER_DEBUG("Number of cluster servers: ", clusters.size());

	while(!clusters.empty()){
		it = clusters.begin() + static_cast<std::ptrdiff_t>(Poseidon::rand32(0, clusters.size()));
_use_hint:
		const auto cluster_coord = it->first;
		LOG_EMPERY_CENTER_DEBUG("Trying cluster server: cluster_coord = ", cluster_coord);
		auto castle = place_castle_random_restricted(factory, cluster_coord);
		if(castle){
			LOG_EMPERY_CENTER_INFO("Castle placed successfully: map_object_uuid = ", castle->get_map_object_uuid(),
				", owner_uuid = ", castle->get_owner_uuid(), ", coord = ", castle->get_coord());
			return std::move(castle);
		}
		clusters.erase(it);
	}
	return { };
}

}
