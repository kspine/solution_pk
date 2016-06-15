#include "../precompiled.hpp"
#include "world_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/async_job.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "player_session_map.hpp"
#include "map_event_block_map.hpp"
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
#include "../overlay.hpp"
#include "../mysql/overlay.hpp"
#include "../strategic_resource.hpp"
#include "../mysql/strategic_resource.hpp"
#include "../resource_crate.hpp"
#include "../mysql/resource_crate.hpp"
#include "../player_session.hpp"
#include "../cluster_session.hpp"
#include "../map_utilities_center.hpp"
#include "../map_event_block.hpp"
#include "../account.hpp"
#include "../account_attribute_ids.hpp"
#include "controller_client.hpp"
#include "../msg/st_map.hpp"

namespace EmperyCenter {

namespace {
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
/*
	void map_cell_refresh_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Map cell refresh timer: now = ", now);

		const auto map_cell_map = g_map_cell_map.lock();
		if(!map_cell_map){
			return;
		}

		std::vector<boost::shared_ptr<MapCell>> map_cells_to_pump;
		const auto range = std::make_pair(map_cell_map->upper_bound<3>(false), map_cell_map->end<3>());
		map_cells_to_pump.reserve(static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			const auto &map_cell = it->map_cell;
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
	}
*/
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
	}

	inline MapObjectUuid get_garrisoning_object_uuid_aux(const boost::shared_ptr<MapObject> map_object){
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
			, garrisoning_object_uuid(get_garrisoning_object_uuid_aux(map_object))
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

	struct OverlayElement {
		boost::shared_ptr<Overlay> overlay;

		std::pair<Coord, SharedNts> cluster_coord_group_name;

		explicit OverlayElement(boost::shared_ptr<Overlay> overlay_)
			: overlay(std::move(overlay_))
			, cluster_coord_group_name(overlay->get_cluster_coord(), SharedNts(overlay->get_overlay_group_name()))
		{
		}
	};

	MULTI_INDEX_MAP(OverlayContainer, OverlayElement,
		UNIQUE_MEMBER_INDEX(cluster_coord_group_name)
	)

	boost::weak_ptr<OverlayContainer> g_overlay_map;

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

	enum : unsigned {
		MAP_WIDTH          = 600,
		MAP_HEIGHT         = 640,

		SECTOR_SIDE_LENGTH = 32,
	};

	inline Coord get_sector_coord_from_world_coord(Coord coord){
		const auto mask_x = coord.x() >> 63;
		const auto mask_y = coord.y() >> 63;

		const auto sector_x = ((coord.x() ^ mask_x) / SECTOR_SIDE_LENGTH ^ mask_x) * SECTOR_SIDE_LENGTH;
		const auto sector_y = ((coord.y() ^ mask_y) / SECTOR_SIDE_LENGTH ^ mask_y) * SECTOR_SIDE_LENGTH;

		return Coord(sector_x, sector_y);
	}

	struct PlayerViewElement {
		boost::weak_ptr<PlayerSession> session;
		Coord sector_coord;

		PlayerViewElement(boost::weak_ptr<PlayerSession> session_, Coord sector_coord_)
			: session(std::move(session_)), sector_coord(sector_coord_)
		{
		}
	};

	MULTI_INDEX_MAP(PlayerViewContainer, PlayerViewElement,
		MULTI_MEMBER_INDEX(session)
		MULTI_MEMBER_INDEX(sector_coord)
	)

	boost::weak_ptr<PlayerViewContainer> g_player_view_map;

	inline Coord get_cluster_coord_from_world_coord(Coord coord){
		const auto mask_x = coord.x() >> 63;
		const auto mask_y = coord.y() >> 63;

		const auto cluster_x = ((coord.x() ^ mask_x) / MAP_WIDTH  ^ mask_x) * MAP_WIDTH;
		const auto cluster_y = ((coord.y() ^ mask_y) / MAP_HEIGHT ^ mask_y) * MAP_HEIGHT;

		return Coord(cluster_x, cluster_y);
	}

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

	struct TempObjectElement {
		// MapObject
		boost::shared_ptr<MySql::Center_MapObject> obj;
		std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> attributes;
		std::vector<boost::shared_ptr<MySql::Center_MapObjectBuff>> buffs;
		// DefenseBuilding
		std::vector<boost::shared_ptr<MySql::Center_DefenseBuilding>> defense_objs;
		// Castle
		std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> buildings;
		std::vector<boost::shared_ptr<MySql::Center_CastleTech>> techs;
		std::vector<boost::shared_ptr<MySql::Center_CastleResource>> resources;
		std::vector<boost::shared_ptr<MySql::Center_CastleBattalion>> soldiers;
		std::vector<boost::shared_ptr<MySql::Center_CastleBattalionProduction>> soldier_production;
		std::vector<boost::shared_ptr<MySql::Center_CastleWoundedSoldier>> wounded_soldiers;
		std::vector<boost::shared_ptr<MySql::Center_CastleTreatment>> treatment;
	};

	boost::shared_ptr<MapObject> create_map_object_aux(const TempObjectElement &elem){
		PROFILE_ME;

		switch(elem.obj->get_map_object_type_id()){
		case MapObjectTypeIds::ID_CASTLE.get():
			return boost::make_shared<Castle>(std::move(elem.obj), elem.attributes, elem.buffs, elem.defense_objs,
				elem.buildings, elem.techs, elem.resources, elem.soldiers, elem.soldier_production, elem.wounded_soldiers, elem.treatment);
		case MapObjectTypeIds::ID_DEFENSE_TOWER.get():
		case MapObjectTypeIds::ID_BATTLE_BUNKER.get():
			return boost::make_shared<DefenseBuilding>(std::move(elem.obj), elem.attributes, elem.buffs, elem.defense_objs);
		default:
			return boost::make_shared<MapObject>(std::move(elem.obj), elem.attributes, elem.buffs);
		}
	}

	boost::shared_ptr<MapObject> reload_map_object_aux(boost::shared_ptr<MySql::Center_MapObject> obj){
		PROFILE_ME;

		TempObjectElement elem = { };
		elem.obj = std::move(obj);

#define RELOAD_PART_(sink_, table_)	\
			{	\
				std::ostringstream oss;	\
				const auto &map_object_uuid = obj->unlocked_get_map_object_uuid();	\
				oss <<"SELECT * FROM `" #table_ "` WHERE `map_object_uuid` = " <<Poseidon::MySql::UuidFormatter(map_object_uuid);	\
				const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(	\
					[&](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){	\
						auto obj = boost::make_shared<MySql:: table_ >();	\
						obj->fetch(conn);	\
						obj->enable_auto_saving();	\
						(sink_) .emplace_back(std::move(obj));	\
					}, #table_, oss.str());	\
				Poseidon::JobDispatcher::yield(promise, false);	\
			}
		switch(elem.obj->get_map_object_type_id()){
		case MapObjectTypeIds::ID_CASTLE.get():
			RELOAD_PART_(elem.buildings,          Center_CastleBuildingBase)
			RELOAD_PART_(elem.techs,              Center_CastleTech)
			RELOAD_PART_(elem.resources,          Center_CastleResource)
			RELOAD_PART_(elem.soldiers,           Center_CastleBattalion)
			RELOAD_PART_(elem.soldier_production, Center_CastleBattalionProduction)
			RELOAD_PART_(elem.wounded_soldiers,   Center_CastleWoundedSoldier)
			RELOAD_PART_(elem.treatment,          Center_CastleTreatment)
		case MapObjectTypeIds::ID_DEFENSE_TOWER.get():
		case MapObjectTypeIds::ID_BATTLE_BUNKER.get():
			RELOAD_PART_(elem.defense_objs,       Center_DefenseBuilding)
		default:
			RELOAD_PART_(elem.attributes,         Center_MapObjectAttribute)
			RELOAD_PART_(elem.buffs,              Center_MapObjectBuff)
		}
#undef RELOAD_PART_

		return create_map_object_aux(elem);
	}

	MODULE_RAII_PRIORITY(handles, 5300){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// MapCell
		struct TempCellElement {
			boost::shared_ptr<MySql::Center_MapCell> obj;
			std::vector<boost::shared_ptr<MySql::Center_MapCellAttribute>> attributes;
			std::vector<boost::shared_ptr<MySql::Center_MapCellBuff>> buffs;
		};
		std::map<Coord, TempCellElement> temp_cell_map;

		LOG_EMPERY_CENTER_INFO("Loading map cells...");
		conn->execute_sql("SELECT * FROM `Center_MapCell`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapCell>();
			obj->fetch(conn);
			const auto ticket_item_id = ItemId(obj->get_ticket_item_id());
			if(!ticket_item_id){
				continue;
			}
			obj->enable_auto_saving();
			const auto coord = Coord(obj->get_x(), obj->get_y());
			temp_cell_map[coord].obj = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_cell_map.size(), " map cell(s).");

		LOG_EMPERY_CENTER_INFO("Loading map cell attributes...");
		conn->execute_sql("SELECT * FROM `Center_MapCellAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapCellAttribute>();
			obj->fetch(conn);
			const auto coord = Coord(obj->get_x(), obj->get_y());
			const auto it = temp_cell_map.find(coord);
			if(it == temp_cell_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.attributes.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading map cell attributes.");

		LOG_EMPERY_CENTER_INFO("Loading map cell buffs...");
		conn->execute_sql("SELECT * FROM `Center_MapCellBuff`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapCellBuff>();
			obj->fetch(conn);
			const auto coord = Coord(obj->get_x(), obj->get_y());
			const auto it = temp_cell_map.find(coord);
			if(it == temp_cell_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.buffs.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading map cell buffs.");

		const auto map_cell_map = boost::make_shared<MapCellContainer>();
		for(auto it = temp_cell_map.begin(); it != temp_cell_map.end(); ++it){
			auto map_cell = boost::make_shared<MapCell>(std::move(it->second.obj), it->second.attributes, it->second.buffs);
			map_cell_map->insert(MapCellElement(std::move(map_cell)));
		}
		g_map_cell_map = map_cell_map;
		handles.push(map_cell_map);

		// MapObject
		std::map<MapObjectUuid, TempObjectElement> temp_object_map;

		LOG_EMPERY_CENTER_INFO("Loading map objects...");
		conn->execute_sql("SELECT * FROM `Center_MapObject` WHERE `deleted` = 0");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapObject>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
			temp_object_map[map_object_uuid].obj = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_object_map.size(), " map object(s).");

		LOG_EMPERY_CENTER_INFO("Loading map object attributes...");
		conn->execute_sql("SELECT * FROM `Center_MapObjectAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapObjectAttribute>();
			obj->fetch(conn);
			const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
			const auto it = temp_object_map.find(map_object_uuid);
			if(it == temp_object_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.attributes.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading map object attributes.");

		LOG_EMPERY_CENTER_INFO("Loading map object buffs...");
		conn->execute_sql("SELECT * FROM `Center_MapObjectBuff`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapObjectBuff>();
			obj->fetch(conn);
			const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
			const auto it = temp_object_map.find(map_object_uuid);
			if(it == temp_object_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.buffs.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading map object buffs.");

		LOG_EMPERY_CENTER_INFO("Loading defense buildings...");
		conn->execute_sql("SELECT * FROM `Center_DefenseBuilding`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_DefenseBuilding>();
			obj->fetch(conn);
			const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
			const auto it = temp_object_map.find(map_object_uuid);
			if(it == temp_object_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.defense_objs.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading defense buildings.");

		LOG_EMPERY_CENTER_INFO("Loading castle building bases...");
		conn->execute_sql("SELECT * FROM `Center_CastleBuildingBase`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_CastleBuildingBase>();
			obj->fetch(conn);
			const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
			const auto it = temp_object_map.find(map_object_uuid);
			if(it == temp_object_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.buildings.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading castle buildings.");

		LOG_EMPERY_CENTER_INFO("Loading castle tech...");
		conn->execute_sql("SELECT * FROM `Center_CastleTech`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_CastleTech>();
			obj->fetch(conn);
			const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
			const auto it = temp_object_map.find(map_object_uuid);
			if(it == temp_object_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.techs.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading castle techs.");

		LOG_EMPERY_CENTER_INFO("Loading castle resource...");
		conn->execute_sql("SELECT * FROM `Center_CastleResource`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_CastleResource>();
			obj->fetch(conn);
			const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
			const auto it = temp_object_map.find(map_object_uuid);
			if(it == temp_object_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.resources.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading castle resources.");

		LOG_EMPERY_CENTER_INFO("Loading castle soldiers...");
		conn->execute_sql("SELECT * FROM `Center_CastleBattalion`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_CastleBattalion>();
			obj->fetch(conn);
			const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
			const auto it = temp_object_map.find(map_object_uuid);
			if(it == temp_object_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.soldiers.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading castle soldiers.");

		LOG_EMPERY_CENTER_INFO("Loading castle soldier production...");
		conn->execute_sql("SELECT * FROM `Center_CastleBattalionProduction`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_CastleBattalionProduction>();
			obj->fetch(conn);
			const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
			const auto it = temp_object_map.find(map_object_uuid);
			if(it == temp_object_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.soldier_production.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading castle soldier production.");

		LOG_EMPERY_CENTER_INFO("Loading castle wounded soldiers...");
		conn->execute_sql("SELECT * FROM `Center_CastleWoundedSoldier`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_CastleWoundedSoldier>();
			obj->fetch(conn);
			const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
			const auto it = temp_object_map.find(map_object_uuid);
			if(it == temp_object_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.wounded_soldiers.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading castle wounded soldiers.");

		LOG_EMPERY_CENTER_INFO("Loading castle treatment...");
		conn->execute_sql("SELECT * FROM `Center_CastleTreatment`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_CastleTreatment>();
			obj->fetch(conn);
			const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
			const auto it = temp_object_map.find(map_object_uuid);
			if(it == temp_object_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.treatment.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading castle treatment.");

		const auto map_object_map = boost::make_shared<MapObjectContainer>();
		for(auto it = temp_object_map.begin(); it != temp_object_map.end(); ++it){
			auto map_object = create_map_object_aux(it->second);
			map_object_map->insert(MapObjectElement(std::move(map_object)));
		}
		g_map_object_map = map_object_map;
		handles.push(map_object_map);

		// Overlay
		const auto overlay_map = boost::make_shared<OverlayContainer>();
		LOG_EMPERY_CENTER_INFO("Loading overlays...");
		conn->execute_sql("SELECT * FROM `Center_Overlay`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_Overlay>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			auto overlay = boost::make_shared<Overlay>(std::move(obj));
			overlay_map->insert(OverlayElement(std::move(overlay)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", overlay_map->size(), " overlay(s).");
		g_overlay_map = overlay_map;
		handles.push(overlay_map);

		// StrategicResource
		const auto strategic_resource_map = boost::make_shared<StrategicResourceContainer>();
		LOG_EMPERY_CENTER_INFO("Loading strategic resources...");
		conn->execute_sql("SELECT * FROM `Center_StrategicResource` WHERE `resource_amount` > 0");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_StrategicResource>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			auto strategic_resource = boost::make_shared<StrategicResource>(std::move(obj));
			strategic_resource_map->insert(StrategicResourceElement(std::move(strategic_resource)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", strategic_resource_map->size(), " strategic resources(s).");
		g_strategic_resource_map = strategic_resource_map;
		handles.push(strategic_resource_map);

		// ResourceCrate
		const auto resource_crate_map = boost::make_shared<ResourceCrateContainer>();
		LOG_EMPERY_CENTER_INFO("Loading resource crates...");
		conn->execute_sql("SELECT * FROM `Center_ResourceCrate` WHERE `amount_remaining` > 0");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_ResourceCrate>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			auto resource_crate = boost::make_shared<ResourceCrate>(std::move(obj));
			resource_crate_map->insert(ResourceCrateElement(std::move(resource_crate)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", resource_crate_map->size(), " resource crate(s).");
		g_resource_crate_map = resource_crate_map;
		handles.push(resource_crate_map);

		// PlayerSession
		const auto player_view_map = boost::make_shared<PlayerViewContainer>();
		g_player_view_map = player_view_map;
		handles.push(player_view_map);

		// ClusterSession
		const auto cluster_map = boost::make_shared<ClusterContainer>();
		g_cluster_map = cluster_map;
		handles.push(cluster_map);

		// DelayedWorldSynchronization
		const auto synchronization_queue = boost::make_shared<SynchronizationQueue>();
		g_synchronization_queue = synchronization_queue;
		handles.push(synchronization_queue);

		Poseidon::enqueue_async_job(
			[=]{
				PROFILE_ME;
				LOG_EMPERY_CENTER_DEBUG("Recalculating castle attributes...");
				for(auto it = map_object_map->begin(); it != map_object_map->end(); ++it){
					const auto castle = boost::dynamic_pointer_cast<Castle>(it->map_object);
					if(!castle){
						continue;
					}
					castle->check_init_buildings();
					castle->check_init_resources();
					castle->pump_status();
					castle->recalculate_attributes(true);
				}
				LOG_EMPERY_CENTER_DEBUG("Done recalculating castle attributes.");
			});
/*
		const auto map_cell_refresh_interval = get_config<std::uint64_t>("map_cell_refresh_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, map_cell_refresh_interval,
			std::bind(&map_cell_refresh_timer_proc, std::placeholders::_2));
		handles.push(timer);
*/
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

		const auto resource_crate_refresh_interval = get_config<std::uint64_t>("resource_crate_refresh_interval", 300000);
		timer = Poseidon::TimerDaemon::register_timer(0, resource_crate_refresh_interval,
			std::bind(&resource_crate_refresh_timer_proc, std::placeholders::_2));
		handles.push(timer);
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
		const auto map_object_type_id = map_object->get_map_object_type_id();

		if((old_coord != new_coord) && (map_object_type_id == MapObjectTypeIds::ID_CASTLE)){
			const auto castle = boost::dynamic_pointer_cast<Castle>(map_object);
			if(castle){
				try {
					Msg::ST_MapInvalidateCastle msg;
					msg.map_object_uuid = map_object_uuid.str();
					msg.coord_x         = new_coord.x();
					msg.coord_y         = new_coord.y();
					controller->send(msg);
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}
		}

		if(map_object->is_virtually_removed()){
			try {
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

	std::vector<boost::shared_ptr<MySql::Center_MapObject>> sink;
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_MapObject` WHERE `deleted` = 0 AND `map_object_type_id` = " <<EmperyCenter::MapObjectTypeIds::ID_CASTLE
		    <<"  AND `map_object_uuid` = " <<Poseidon::MySql::UuidFormatter(map_object_uuid.get());
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[&](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<EmperyCenter::MySql::Center_MapObject>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink.emplace_back(std::move(obj));
			}, "Center_MapObject", oss.str());
		Poseidon::JobDispatcher::yield(promise, false);
	}
	if(sink.empty()){
		LOG_EMPERY_CENTER_DEBUG("Castle not found in database: map_object_uuid = ", map_object_uuid);
		map_object_map->erase<0>(map_object_uuid);
		return { };
	}

	auto map_object = reload_map_object_aux(std::move(sink.front()));

	const auto elem = MapObjectElement(map_object);
	const auto result = map_object_map->insert(elem);
	if(result.second){
		map_object_map->replace(result.first, elem);
	}

	LOG_EMPERY_CENTER_DEBUG("Successfully reloaded map object: map_object_uuid = ", map_object_uuid);
	return std::move(map_object);
}
void WorldMap::forced_reload_child_map_objects(MapObjectUuid parent_object_uuid){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return;
	}

	std::vector<boost::shared_ptr<MySql::Center_MapObject>> sink;
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_MapObject` WHERE `deleted` = 0 AND `map_object_type_id` != " <<EmperyCenter::MapObjectTypeIds::ID_CASTLE
		    <<"  AND `parent_object_uuid` = " <<Poseidon::MySql::UuidFormatter(parent_object_uuid.get());
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[&](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<EmperyCenter::MySql::Center_MapObject>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink.emplace_back(std::move(obj));
			}, "Center_MapObject", oss.str());
		Poseidon::JobDispatcher::yield(promise, false);
	}
	if(sink.empty()){
		LOG_EMPERY_CENTER_DEBUG("Castle not found in database: parent_object_uuid = ", parent_object_uuid);
		map_object_map->erase<3>(parent_object_uuid);
		return;
	}

	for(auto it = sink.begin(); it != sink.end(); ++it){
		try {
			auto map_object = reload_map_object_aux(std::move(*it));
			const auto map_object_uuid = map_object->get_map_object_uuid();

			const auto elem = MapObjectElement(std::move(map_object));
			const auto result = map_object_map->insert(elem);
			if(result.second){
				map_object_map->replace(result.first, elem);
			}

			LOG_EMPERY_CENTER_DEBUG("Successfully reloaded map object: map_object_uuid = ", map_object_uuid);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}
}
void WorldMap::insert_map_object(const boost::shared_ptr<MapObject> &map_object){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		DEBUG_THROW(Exception, sslit("Map object map not loaded"));
	}

	const auto map_object_uuid = map_object->get_map_object_uuid();

	if(map_object->has_been_deleted()){
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
	if(map_object->is_virtually_removed()){
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
void WorldMap::get_map_objects_by_garrisoning_object(std::vector<boost::shared_ptr<MapObject>> &ret, MapObjectUuid garrisoning_object_uuid){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return;
	}

	const auto range = map_object_map->equal_range<4>(garrisoning_object_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->map_object);
	}
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
MapObjectUuid WorldMap::get_primary_castle_uuid(AccountUuid owner_uuid){
	PROFILE_ME;

	MapObjectUuid min_castle_uuid;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return min_castle_uuid;
	}

	const auto range = map_object_map->equal_range<2>(owner_uuid);
	for(auto it = range.first; it != range.second; ++it){
		const auto &map_object = it->map_object;
		if(map_object->get_map_object_type_id() != MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		const auto castle_uuid = map_object->get_map_object_uuid();
		if(min_castle_uuid && (min_castle_uuid <= castle_uuid)){
			continue;
		}
		min_castle_uuid = castle_uuid;
	}

	return min_castle_uuid;
}

boost::shared_ptr<Overlay> WorldMap::get_overlay(Coord cluster_coord, const std::string &overlay_group_name){
	PROFILE_ME;

	const auto overlay_map = g_overlay_map.lock();
	if(!overlay_map){
		LOG_EMPERY_CENTER_WARNING("Overlay map not loaded.");
		return { };
	}

	const auto it = overlay_map->find<0>(std::make_pair(cluster_coord, SharedNts::view(overlay_group_name.c_str())));
	if(it == overlay_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Overlay not found: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name);
		return { };
	}
	return it->overlay;
}
boost::shared_ptr<Overlay> WorldMap::require_overlay(Coord cluster_coord, const std::string &overlay_group_name){
	PROFILE_ME;

	auto ret = get_overlay(cluster_coord, overlay_group_name);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Overlay not found: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name);
		DEBUG_THROW(Exception, sslit("Overlay not found"));
	}
	return ret;
}
void WorldMap::insert_overlay(const boost::shared_ptr<Overlay> &overlay){
	PROFILE_ME;

	const auto overlay_map = g_overlay_map.lock();
	if(!overlay_map){
		LOG_EMPERY_CENTER_WARNING("Overlay map not loaded.");
		DEBUG_THROW(Exception, sslit("Overlay map not loaded"));
	}

	const auto cluster_coord = overlay->get_cluster_coord();
	const auto &overlay_group_name = overlay->get_overlay_group_name();

	std::vector<boost::shared_ptr<const Data::MapCellBasic>> basic_data_elements;
	Data::MapCellBasic::get_by_overlay_group(basic_data_elements, overlay_group_name);
	if(basic_data_elements.empty()){
		LOG_EMPERY_CENTER_ERROR("Overlay group not found: overlay_group_name = ", overlay_group_name);
		DEBUG_THROW(Exception, sslit("Overlay group not found"));
	}
	std::uint64_t sum_x = 0, sum_y = 0;
	for(auto it = basic_data_elements.begin(); it != basic_data_elements.end(); ++it){
		const auto &basic_data = *it;
		sum_x += basic_data->map_coord.first;
		sum_y += basic_data->map_coord.second;
	}
	const auto coord = Coord(cluster_coord.x() + static_cast<std::int64_t>(sum_x / basic_data_elements.size()),
	                         cluster_coord.y() + static_cast<std::int64_t>(sum_y / basic_data_elements.size()));

	LOG_EMPERY_CENTER_TRACE("Inserting overlay: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name);
	const auto result = overlay_map->insert(OverlayElement(overlay));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Overlay already exists: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name);
		DEBUG_THROW(Exception, sslit("Overlay already exists"));
	}

	synchronize_with_all_players(overlay, coord, coord, { });
}
void WorldMap::update_overlay(const boost::shared_ptr<Overlay> &overlay, bool throws_if_not_exists){
	PROFILE_ME;

	const auto overlay_map = g_overlay_map.lock();
	if(!overlay_map){
		LOG_EMPERY_CENTER_WARNING("Overlay map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Overlay map not loaded"));
		}
		return;
	}

	const auto cluster_coord = overlay->get_cluster_coord();
	const auto &overlay_group_name = overlay->get_overlay_group_name();

	std::vector<boost::shared_ptr<const Data::MapCellBasic>> basic_data_elements;
	Data::MapCellBasic::get_by_overlay_group(basic_data_elements, overlay_group_name);
	if(basic_data_elements.empty()){
		LOG_EMPERY_CENTER_ERROR("Overlay group not found: overlay_group_name = ", overlay_group_name);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Overlay group not found"));
		}
		return;
	}
	std::uint64_t sum_x = 0, sum_y = 0;
	for(auto it = basic_data_elements.begin(); it != basic_data_elements.end(); ++it){
		const auto &basic_data = *it;
		sum_x += basic_data->map_coord.first;
		sum_y += basic_data->map_coord.second;
	}
	const auto coord = Coord(cluster_coord.x() + static_cast<std::int64_t>(sum_x / basic_data_elements.size()),
	                         cluster_coord.y() + static_cast<std::int64_t>(sum_y / basic_data_elements.size()));

	const auto it = overlay_map->find<0>(std::make_pair(cluster_coord, SharedNts::view(overlay_group_name.c_str())));
	if(it == overlay_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Overlay not found: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Overlay not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating overlay: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name);
	if(overlay->is_virtually_removed()){
		overlay_map->erase<0>(it);
	} else {
		overlay_map->replace<0>(it, OverlayElement(overlay));
	}

	synchronize_with_all_players(overlay, coord, coord, { });
}

void WorldMap::get_overlays_by_rectangle(std::vector<boost::shared_ptr<Overlay>> &ret, Rectangle rectangle){
	PROFILE_ME;

	boost::container::flat_set<boost::shared_ptr<Overlay>> temp;
	for(auto x = rectangle.left(); x < rectangle.right(); ++x){
		for(auto y = rectangle.bottom(); y < rectangle.top(); ++y){
			const auto cluster_coord = get_cluster_coord_from_world_coord(Coord(x, y));
			const auto map_x = static_cast<unsigned>(x - cluster_coord.x());
			const auto map_y = static_cast<unsigned>(y - cluster_coord.y());
			const auto basic_data = Data::MapCellBasic::require(map_x, map_y);
			if(!basic_data->overlay_group_name.empty()){
				auto overlay = get_overlay(cluster_coord, basic_data->overlay_group_name);
				if(overlay){
					temp.insert(std::move(overlay));
				}
			}
		}
	}
	ret.reserve(ret.size() + temp.size());
	std::move(temp.begin(), temp.end(), std::back_inserter(ret));
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

	std::vector<boost::shared_ptr<Overlay>> overlays;
	get_overlays_by_rectangle(overlays, view);
	for(auto it = overlays.begin(); it != overlays.end(); ++it){
		const auto &overlay = *it;
		if(overlay->is_virtually_removed()){
			continue;
		}
		synchronize_overlay_with_player(overlay, session);
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
void WorldMap::get_all_clusters(boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> &ret){
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
		ret.insert(std::make_pair(it->cluster_coord, std::move(cluster)));
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
	LOG_EMPERY_CENTER_INFO("Initiating map for cluster server: scope = ", scope);

	for(unsigned map_y = 0; map_y < scope.height(); ++map_y){
		for(unsigned map_x = 0; map_x < scope.width(); ++map_x){
			const auto coord = Coord(scope.left() + map_x, scope.bottom() + map_y);
/*
			auto map_cell = get_map_cell(coord);
			if(!map_cell){
				map_cell = boost::make_shared<MapCell>(coord);
				map_cell->pump_status();
				insert_map_cell(map_cell);
			}
*/
			const auto basic_data = Data::MapCellBasic::require(map_x, map_y);
			if(!basic_data->overlay_group_name.empty() && basic_data->overlay_id){
				auto overlay = get_overlay(cluster_coord, basic_data->overlay_group_name);
				if(!overlay){
					std::vector<boost::shared_ptr<const Data::MapCellBasic>> cells_in_group;
					Data::MapCellBasic::get_by_overlay_group(cells_in_group, basic_data->overlay_group_name);
					const auto overlay_data = Data::MapOverlay::require(basic_data->overlay_id);

					std::uint64_t resource_amount = 0;
					for(auto it = cells_in_group.begin(); it != cells_in_group.end(); ++it){
						const auto &basic_data = *it;
						const auto overlay_data = Data::MapOverlay::require(basic_data->overlay_id);
						resource_amount = checked_add(resource_amount, overlay_data->reward_resource_amount);
					}
					overlay = boost::make_shared<Overlay>(cluster_coord, basic_data->overlay_group_name,
						basic_data->overlay_id, overlay_data->reward_resource_id, resource_amount);
					overlay->pump_status();
					insert_overlay(overlay);
				}
			}

			const auto event_block_coord = MapEventBlockMap::get_block_coord_from_world_coord(coord);
			auto map_event_block = MapEventBlockMap::get(event_block_coord);
			if(!map_event_block){
				map_event_block = boost::make_shared<MapEventBlock>(event_block_coord);
				map_event_block->pump_status();
				MapEventBlockMap::insert(map_event_block);
			}
		}
	}

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

boost::shared_ptr<Castle> WorldMap::create_init_castle_restricted(
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
			LOG_EMPERY_CENTER_DEBUG("Try creating init castle: coord = ", coord);

			const auto result = can_deploy_castle_at(coord, { });
			if(result.first == 0){
				castle = factory(coord);
				assert(castle);
				castle->pump_status();
				castle->recalculate_attributes(false);

				LOG_EMPERY_CENTER_DEBUG("Creating castle: coord = ", coord);
				map_object_map->insert(MapObjectElement(castle));
				update_map_object(castle, false);
			}
			cached_start_points.erase(cached_start_points.begin() + static_cast<std::ptrdiff_t>(index));

			if(castle){
				LOG_EMPERY_CENTER_DEBUG("Init castle created successfully: map_object_uuid = ", castle->get_map_object_uuid(),
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
boost::shared_ptr<Castle> WorldMap::create_init_castle(
	const boost::function<boost::shared_ptr<Castle> (Coord)> &factory, Coord coord_hint)
{
	PROFILE_ME;

	boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> clusters;
	get_all_clusters(clusters);

	const auto cluster_coord_hint = get_cluster_coord_from_world_coord(coord_hint);
	auto it = clusters.find(cluster_coord_hint);
	if(it != clusters.end()){
		goto _use_hint;
	}
	LOG_EMPERY_CENTER_DEBUG("Number of cluster servers: ", clusters.size());
	while(!clusters.empty()){
		it = clusters.begin() + static_cast<std::ptrdiff_t>(Poseidon::rand32(0, clusters.size()));
_use_hint:
		const auto cluster_coord = it->first;
		LOG_EMPERY_CENTER_DEBUG("Trying cluster server: cluster_coord = ", cluster_coord);
		auto castle = create_init_castle_restricted(factory, cluster_coord);
		if(castle){
			LOG_EMPERY_CENTER_INFO("Init castle created successfully: map_object_uuid = ", castle->get_map_object_uuid(),
				", owner_uuid = ", castle->get_owner_uuid(), ", coord = ", castle->get_coord());
			castle->check_init_buildings();
			castle->check_init_resources();
			return std::move(castle);
		}
		clusters.erase(it);
	}
	return { };
}

}
