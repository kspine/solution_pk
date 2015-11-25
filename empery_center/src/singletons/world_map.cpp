#include "../precompiled.hpp"
#include "world_map.hpp"
#include <boost/container/flat_set.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../map_cell.hpp"
#include "../mysql/map_cell.hpp"
#include "../map_object.hpp"
#include "../map_object_type_ids.hpp"
#include "../mysql/map_object.hpp"
#include "../mysql/castle.hpp"
#include "../castle.hpp"
#include "../player_session.hpp"
#include "../cluster_session.hpp"
#include "../msg/ck_map.hpp"

namespace EmperyCenter {

namespace {
	struct MapCellElement {
		boost::shared_ptr<MapCell> map_cell;

		Coord coord;
		MapObjectUuid parent_object_uuid;

		explicit MapCellElement(boost::shared_ptr<MapCell> map_cell_)
			: map_cell(std::move(map_cell_))
			, coord(map_cell->get_coord()), parent_object_uuid(map_cell->get_parent_object_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(MapCellMapDelegator, MapCellElement,
		UNIQUE_MEMBER_INDEX(coord)
		MULTI_MEMBER_INDEX(parent_object_uuid)
	)

	boost::weak_ptr<MapCellMapDelegator> g_map_cell_map;

	struct MapObjectElement {
		boost::shared_ptr<MapObject> map_object;

		MapObjectUuid map_object_uuid;
		Coord coord;
		AccountUuid owner_uuid;
		MapObjectUuid parent_object_uuid;

		explicit MapObjectElement(boost::shared_ptr<MapObject> map_object_)
			: map_object(std::move(map_object_))
			, map_object_uuid(map_object->get_map_object_uuid()), coord(map_object->get_coord())
			, owner_uuid(map_object->get_owner_uuid()), parent_object_uuid(map_object->get_parent_object_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(MapObjectMapDelegator, MapObjectElement,
		UNIQUE_MEMBER_INDEX(map_object_uuid)
		MULTI_MEMBER_INDEX(coord)
		MULTI_MEMBER_INDEX(owner_uuid)
		MULTI_MEMBER_INDEX(parent_object_uuid)
	)

	boost::weak_ptr<MapObjectMapDelegator> g_map_object_map;

	struct MapSectorElement {
		Coord sector_coord;

		mutable boost::container::flat_set<boost::weak_ptr<MapObject>> map_objects;

		explicit MapSectorElement(Coord sector_coord_)
			: sector_coord(sector_coord_)
		{
		}
	};

	MULTI_INDEX_MAP(MapSectorMapDelegator, MapSectorElement,
		UNIQUE_MEMBER_INDEX(sector_coord)
	)

	boost::weak_ptr<MapSectorMapDelegator> g_map_sector_map;

	struct PlayerViewElement {
		Rectangle view;

		boost::weak_ptr<PlayerSession> session;
		Coord sector_coord;

		PlayerViewElement(Rectangle view_,
			boost::weak_ptr<PlayerSession> session_, Coord sector_coord_)
			: view(view_)
			, session(std::move(session_)), sector_coord(sector_coord_)
		{
		}
	};

	MULTI_INDEX_MAP(PlayerViewMapDelegator, PlayerViewElement,
		MULTI_MEMBER_INDEX(session)
		MULTI_MEMBER_INDEX(sector_coord)
	)

	boost::weak_ptr<PlayerViewMapDelegator> g_player_view_map;

	boost::uint32_t g_map_width  = 270;
	boost::uint32_t g_map_height = 240;

	struct ClusterElement {
		Rectangle scope;

		Coord coord;
		boost::weak_ptr<ClusterSession> cluster;

		ClusterElement(Rectangle scope_, boost::weak_ptr<ClusterSession> cluster_)
			: scope(scope_)
			, coord(scope.bottom_left()), cluster(std::move(cluster_))
		{
		}
	};

	MULTI_INDEX_MAP(ClusterMapDelegator, ClusterElement,
		UNIQUE_MEMBER_INDEX(coord)
		UNIQUE_MEMBER_INDEX(cluster)
	)

	boost::weak_ptr<ClusterMapDelegator> g_cluster_map;

	inline Coord get_sector_coord_from_world_coord(Coord coord){
		return Coord(coord.x() & -32, coord.y() & -32);
	}
	inline Coord get_cluster_coord_from_world_coord(Coord coord){
		const auto mask_x = (coord.x() >= 0) ? 0 : -1;
		const auto mask_y = (coord.y() >= 0) ? 0 : -1;

		const auto cluster_x = ((coord.x() ^ mask_x) / g_map_width  ^ mask_x) * g_map_width;
		const auto cluster_y = ((coord.y() ^ mask_y) / g_map_height ^ mask_y) * g_map_height;

		return Coord(cluster_x, cluster_y);
	}

	MODULE_RAII_PRIORITY(handles, 5300){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// MapCell
		struct TempMapCellElement {
			boost::shared_ptr<MySql::Center_MapCell> obj;
			std::vector<boost::shared_ptr<MySql::Center_MapCellAttribute>> attributes;
		};
		std::map<Coord, TempMapCellElement> temp_map_cell_map;

		LOG_EMPERY_CENTER_INFO("Loading map cells...");
		conn->execute_sql("SELECT * FROM `Center_MapCell`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapCell>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			const auto coord = Coord(obj->get_x(), obj->get_y());
			temp_map_cell_map[coord].obj = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_map_cell_map.size(), " map cell(s).");

		LOG_EMPERY_CENTER_INFO("Loading map cell attributes...");
		conn->execute_sql("SELECT * FROM `Center_MapCellAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapCellAttribute>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			const auto coord = Coord(obj->get_x(), obj->get_y());
			const auto it = temp_map_cell_map.find(coord);
			if(it == temp_map_cell_map.end()){
				continue;
			}
			it->second.attributes.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading map cell attributes.");

		const auto map_cell_map = boost::make_shared<MapCellMapDelegator>();
		for(auto it = temp_map_cell_map.begin(); it != temp_map_cell_map.end(); ++it){
			auto map_cell = boost::make_shared<MapCell>(std::move(it->second.obj), it->second.attributes);

			map_cell_map->insert(MapCellElement(std::move(map_cell)));
		}
		g_map_cell_map = map_cell_map;
		handles.push(map_cell_map);

		// MapObject
		struct TempMapObjectElement {
			boost::shared_ptr<MySql::Center_MapObject> obj;
			std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> attributes;
		};
		std::map<Poseidon::Uuid, TempMapObjectElement> temp_map_object_map;

		LOG_EMPERY_CENTER_INFO("Loading map objects...");
		conn->execute_sql("SELECT * FROM `Center_MapObject` WHERE `deleted` = 0");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapObject>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			const auto map_object_uuid = obj->unlocked_get_map_object_uuid();
			temp_map_object_map[map_object_uuid].obj = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_map_object_map.size(), " map object(s).");

		LOG_EMPERY_CENTER_INFO("Loading map object attributes...");
		conn->execute_sql("SELECT * FROM `Center_MapObjectAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapObjectAttribute>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			const auto map_object_uuid = obj->unlocked_get_map_object_uuid();
			const auto it = temp_map_object_map.find(map_object_uuid);
			if(it == temp_map_object_map.end()){
				continue;
			}
			it->second.attributes.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading map object attributes.");

		struct TempCastleElement {
			std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> buildings;
			std::vector<boost::shared_ptr<MySql::Center_CastleTech>> techs;
			std::vector<boost::shared_ptr<MySql::Center_CastleResource>> resources;
		};
		const TempCastleElement empty_castle = { };
		std::map<Poseidon::Uuid, TempCastleElement> temp_castle_map;

		LOG_EMPERY_CENTER_INFO("Loading castle building bases...");
		conn->execute_sql("SELECT * FROM `Center_CastleBuildingBase`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_CastleBuildingBase>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			const auto map_object_uuid = obj->unlocked_get_map_object_uuid();
			const auto it = temp_map_object_map.find(map_object_uuid);
			if(it == temp_map_object_map.end()){
				continue;
			}
			temp_castle_map[map_object_uuid].buildings.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_castle_map.size(), " castle(s).");

		LOG_EMPERY_CENTER_INFO("Loading castle tech...");
		conn->execute_sql("SELECT * FROM `Center_CastleTech`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_CastleTech>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			const auto map_object_uuid = obj->unlocked_get_map_object_uuid();
			temp_castle_map[map_object_uuid].techs.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading castle techs.");

		LOG_EMPERY_CENTER_INFO("Loading castle resource...");
		conn->execute_sql("SELECT * FROM `Center_CastleResource`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_CastleResource>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			const auto map_object_uuid = obj->unlocked_get_map_object_uuid();
			temp_castle_map[map_object_uuid].resources.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading castle resources.");

		const auto map_object_map = boost::make_shared<MapObjectMapDelegator>();
		for(auto it = temp_map_object_map.begin(); it != temp_map_object_map.end(); ++it){
			boost::shared_ptr<MapObject> map_object;

			const auto map_object_type_id = MapObjectTypeId(it->second.obj->get_map_object_type_id());
			if(map_object_type_id == MapObjectTypeIds::ID_CASTLE){
				const TempCastleElement *elem;
				const auto cit = temp_castle_map.find(it->first);
				if(cit == temp_castle_map.end()){
					elem = &empty_castle;
				} else {
					elem = &cit->second;
				}
				map_object = boost::make_shared<Castle>(std::move(it->second.obj), it->second.attributes,
					elem->buildings, elem->techs, elem->resources);
			} else {
				map_object = boost::make_shared<MapObject>(std::move(it->second.obj), it->second.attributes);
			}

			map_object_map->insert(MapObjectElement(std::move(map_object)));
		}
		g_map_object_map = map_object_map;
		handles.push(map_object_map);

		// PlayerSession
		const auto map_sector_map = boost::make_shared<MapSectorMapDelegator>();
		for(auto it = map_object_map->begin(); it != map_object_map->end(); ++it){
			auto map_object = it->map_object;

			const auto coord = map_object->get_coord();
			const auto sector_coord = get_sector_coord_from_world_coord(coord);
			auto sector_it = map_sector_map->find<0>(sector_coord);
			if(sector_it == map_sector_map->end<0>()){
				sector_it = map_sector_map->insert<0>(sector_it, MapSectorElement(sector_coord));
			}
			sector_it->map_objects.insert(std::move(map_object));
		}
		g_map_sector_map = map_sector_map;
		handles.push(map_sector_map);

		const auto player_view_map = boost::make_shared<PlayerViewMapDelegator>();
		g_player_view_map = player_view_map;
		handles.push(player_view_map);

		// ClusterSession
		get_config(g_map_width,  "map_width");
		get_config(g_map_height, "map_height");
		LOG_EMPERY_CENTER_DEBUG("> Map width = ", g_map_width, ", map height = ", g_map_height);

		const auto cluster_map = boost::make_shared<ClusterMapDelegator>();
		g_cluster_map = cluster_map;
		handles.push(cluster_map);
	}

	void get_sessions_by_coord(std::vector<boost::shared_ptr<PlayerSession>> &ret, Coord coord){
		PROFILE_ME;

		const auto player_view_map = g_player_view_map.lock();
		if(!player_view_map){
			LOG_EMPERY_CENTER_WARNING("Player view map not initialized.");
			return;
		}

		const auto sector_coord = get_sector_coord_from_world_coord(coord);
		const auto range = player_view_map->equal_range<1>(sector_coord);
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		auto view_it = range.first;
		while(view_it != range.second){
			auto session = view_it->session.lock();
			if(!session){
				view_it = player_view_map->erase<1>(view_it);
				continue;
			}
			if(view_it->view.hit_test(coord)){
				ret.emplace_back(std::move(session));
			}
			++view_it;
		}
	}
	void synchronise_map_cell_by_coord(const boost::shared_ptr<MapCell> &map_cell, Coord coord) noexcept {
		PROFILE_ME;

		const auto player_view_map = g_player_view_map.lock();
		if(!player_view_map){
			LOG_EMPERY_CENTER_WARNING("Player view map not initialized.");
			return;
		}

		const auto sector_coord = get_sector_coord_from_world_coord(coord);
		const auto range = player_view_map->equal_range<1>(sector_coord);
		auto view_it = range.first;
		while(view_it != range.second){
			const auto session = view_it->session.lock();
			if(!session){
				view_it = player_view_map->erase<1>(view_it);
				continue;
			}
			if(view_it->view.hit_test(coord)){
				try {
					map_cell->synchronize_with_client(session);
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
					session->shutdown(e.what());
				}
			}
			++view_it;
		}
	}
	void synchronize_map_object_in_sector(const boost::shared_ptr<MapObject> &map_object, Coord sector_coord) noexcept {
		PROFILE_ME;

		const auto player_view_map = g_player_view_map.lock();
		if(!player_view_map){
			LOG_EMPERY_CENTER_WARNING("Player view map not initialized.");
			return;
		}

        const auto coord = map_object->get_coord();
		const auto range = player_view_map->equal_range<1>(sector_coord);
		auto it = range.first;
		while(it != range.second){
			const auto session = it->session.lock();
			if(session){
				try {
					if(it->view.hit_test(coord)){
						map_object->synchronize_with_client(session);
					}
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
					session->shutdown(e.what());
				}
				++it;
			} else {
				it = player_view_map->erase<1>(it);
			}
		}
	}

	void notify_cluster_map_object_added(const boost::shared_ptr<MapObject> &map_object, const boost::shared_ptr<ClusterSession> &cluster){
		PROFILE_ME;

		boost::container::flat_map<AttributeId, boost::int64_t> attributes;
		map_object->get_attributes(attributes);

		Msg::CK_MapAddMapObject msg;
		msg.map_object_uuid    = map_object->get_map_object_uuid().str();
		msg.map_object_type_id = map_object->get_map_object_type_id().get();
		msg.owner_uuid         = map_object->get_owner_uuid().str();
		msg.x                  = map_object->get_coord().x();
		msg.y                  = map_object->get_coord().y();
		msg.attributes.reserve(attributes.size());
		for(auto it = attributes.begin(); it != attributes.end(); ++it){
			msg.attributes.emplace_back();
			auto &attribute = msg.attributes.back();
			attribute.attribute_id = it->first.get();
			attribute.value        = it->second;
		}
		cluster->send(msg);
	}
	void notify_cluster_map_object_removed(const boost::shared_ptr<MapObject> &map_object, const boost::shared_ptr<ClusterSession> &cluster){
		PROFILE_ME;

		Msg::CK_MapRemoveMapObject msg;
		msg.map_object_uuid = map_object->get_map_object_uuid().str();
		cluster->send(msg);
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
		LOG_EMPERY_CENTER_DEBUG("Map cell not found: coord = ", coord);
		return { };
	}
	return it->map_cell;
}
void WorldMap::insert_map_cell(const boost::shared_ptr<MapCell> &map_cell){
	PROFILE_ME;

	const auto map_cell_map = g_map_cell_map.lock();
	if(!map_cell_map){
		LOG_EMPERY_CENTER_WARNING("Map cell map not loaded.");
		DEBUG_THROW(Exception, sslit("Map cell map not loaded"));
	}

	const auto coord = map_cell->get_coord();

	const auto it = map_cell_map->find<0>(coord);
	if(it != map_cell_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Map cell already exists: coord = ", coord);
		DEBUG_THROW(Exception, sslit("Map cell already exists"));
	}

	LOG_EMPERY_CENTER_DEBUG("Inserting map cell: coord = ", coord);
	map_cell_map->insert(MapCellElement(map_cell));
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
		LOG_EMPERY_CENTER_WARNING("Map cell not found: coord = ", coord);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map object not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating map cell: coord = ", coord);

	synchronise_map_cell_by_coord(map_cell, coord);
}

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
void WorldMap::get_map_cells_by_rectangle(boost::container::flat_map<Coord, boost::shared_ptr<MapCell>> &ret, Rectangle rectangle){
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
			ret.emplace(it->coord, it->map_cell);
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
		LOG_EMPERY_CENTER_DEBUG("Map object not found: map_object_uuid = ", map_object_uuid);
		return { };
	}
	return it->map_object;
}
void WorldMap::insert_map_object(const boost::shared_ptr<MapObject> &map_object){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		DEBUG_THROW(Exception, sslit("Map object map not loaded"));
	}
	const auto map_sector_map = g_map_sector_map.lock();
	if(!map_sector_map){
		LOG_EMPERY_CENTER_WARNING("Map sector map not loaded.");
		DEBUG_THROW(Exception, sslit("Map sector map not loaded"));
	}

	const auto map_object_uuid = map_object->get_map_object_uuid();

	if(map_object->has_been_deleted()){
		LOG_EMPERY_CENTER_WARNING("Map object has been marked as deleted: map_object_uuid = ", map_object_uuid);
		DEBUG_THROW(Exception, sslit("Map object has been marked as deleted"));
	}

	const auto it = map_object_map->find<0>(map_object_uuid);
	if(it != map_object_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Map object already exists: map_object_uuid = ", map_object_uuid);
		DEBUG_THROW(Exception, sslit("Map object already exists"));
	}

	const auto new_coord        = map_object->get_coord();
	const auto new_sector_coord = get_sector_coord_from_world_coord(new_coord);
	const auto result = map_sector_map->insert<0>(MapSectorElement(new_sector_coord));
	if(result.second){
		LOG_EMPERY_CENTER_DEBUG("Created map sector: new_sector_coord = ", new_sector_coord);
	}
	const auto new_sector_it    = result.first;
	new_sector_it->map_objects.reserve(new_sector_it->map_objects.size() + 1);

	LOG_EMPERY_CENTER_DEBUG("Inserting map object: map_object_uuid = ", map_object_uuid,
		", new_coord = ", new_coord, ", new_sector_coord = ", new_sector_coord);
	map_object_map->insert(MapObjectElement(map_object));
	new_sector_it->map_objects.insert(map_object); // 确保事先 reserve() 过。

	synchronize_map_object_in_sector(map_object, new_sector_coord);

	const auto new_cluster = get_cluster(new_coord);
	if(new_cluster){
		try {
			notify_cluster_map_object_added(map_object, new_cluster);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			new_cluster->shutdown(e.what());
		}
	}
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
	const auto map_sector_map = g_map_sector_map.lock();
	if(!map_sector_map){
		LOG_EMPERY_CENTER_WARNING("Map sector map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map sector map not loaded"));
		}
		return;
	}

	const auto map_object_uuid = map_object->get_map_object_uuid();

	if(map_object->has_been_deleted()){
		LOG_EMPERY_CENTER_WARNING("Map object has been marked as deleted: map_object_uuid = ", map_object_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map object has been marked as deleted"));
		}
		return;
	}

	const auto it = map_object_map->find<0>(map_object_uuid);
	if(it == map_object_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Map object not found: map_object_uuid = ", map_object_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map object not found"));
		}
		return;
	}
	const auto old_coord        = it->coord;
	const auto old_sector_coord = get_sector_coord_from_world_coord(old_coord);
	const auto old_sector_it    = map_sector_map->find<0>(old_sector_coord);

	const auto new_coord        = map_object->get_coord();
	const auto new_sector_coord = get_sector_coord_from_world_coord(new_coord);
	const auto result = map_sector_map->insert<0>(MapSectorElement(new_sector_coord));
	if(result.second){
		LOG_EMPERY_CENTER_DEBUG("Created map sector: new_sector_coord = ", new_sector_coord);
	}
	const auto new_sector_it    = result.first;
	new_sector_it->map_objects.reserve(new_sector_it->map_objects.size() + 1);

	LOG_EMPERY_CENTER_DEBUG("Updating map object: map_object_uuid = ", map_object_uuid,
		", old_coord = ", old_coord, ", new_coord = ", new_coord,
		", old_sector_coord = ", old_sector_coord, ", new_sector_coord = ", new_sector_coord);
	map_object_map->set_key<0, 1>(it, new_coord);
	if(old_sector_it != map_sector_map->end<0>()){
		old_sector_it->map_objects.erase(map_object); // noexcept
		if(old_sector_it->map_objects.empty()){
			map_sector_map->erase<0>(old_sector_it);
			LOG_EMPERY_CENTER_DEBUG("Removed map sector: old_sector_coord = ", old_sector_coord);
		}
	}
	new_sector_it->map_objects.insert(map_object); // 确保事先 reserve() 过。

	if(old_sector_coord != new_sector_coord){
		synchronize_map_object_in_sector(map_object, old_sector_coord);
	}
	synchronize_map_object_in_sector(map_object, new_sector_coord);

	const auto old_cluster = get_cluster(old_coord);
	const auto new_cluster = get_cluster(new_coord);
	if(old_cluster != new_cluster){
		// 切换地图服务器。
		if(old_cluster){
			try {
				notify_cluster_map_object_removed(map_object, old_cluster);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				old_cluster->shutdown(e.what());
			}
		}
		if(new_cluster){
			try {
				notify_cluster_map_object_added(map_object, new_cluster);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				new_cluster->shutdown(e.what());
			}
		}
	}
}
void WorldMap::remove_map_object(MapObjectUuid map_object_uuid) noexcept {
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CENTER_WARNING("Map object map not loaded.");
		return;
	}
	const auto map_sector_map = g_map_sector_map.lock();
	if(!map_sector_map){
		LOG_EMPERY_CENTER_FATAL("Map sector map not loaded.");
		std::abort();
	}

	const auto it = map_object_map->find<0>(map_object_uuid);
	if(it == map_object_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Map object not found: map_object_uuid = ", map_object_uuid);
		return;
	}
	const auto map_object       = it->map_object;

	const auto old_coord        = it->coord;
	const auto old_sector_coord = get_sector_coord_from_world_coord(old_coord);
	const auto old_sector_it    = map_sector_map->find<0>(old_sector_coord);

	LOG_EMPERY_CENTER_DEBUG("Removing map object: map_object_uuid = ", map_object_uuid,
		", old_coord = ", old_coord, ", old_sector_coord = ", old_sector_coord);
	map_object_map->erase<0>(it);
	if(old_sector_it != map_sector_map->end<0>()){
		old_sector_it->map_objects.erase(map_object); // noexcept
		if(old_sector_it->map_objects.empty()){
			map_sector_map->erase<0>(old_sector_it);
			LOG_EMPERY_CENTER_DEBUG("Removed map sector: old_sector_coord = ", old_sector_coord);
		}
	}

    synchronize_map_object_in_sector(map_object, old_sector_coord);

	const auto old_cluster = get_cluster(old_coord);
	if(old_cluster){
		try {
			notify_cluster_map_object_removed(map_object, old_cluster);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			old_cluster->shutdown(e.what());
		}
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
void WorldMap::get_map_objects_by_rectangle(boost::container::flat_map<Coord, boost::shared_ptr<MapObject>> &ret, Rectangle rectangle){
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
			ret.emplace(it->coord, it->map_object);
			++it;
		}
	}
_exit_while:
	;
}

void WorldMap::get_players_viewing_coord(std::vector<boost::shared_ptr<PlayerSession>> &ret, Coord coord){
	PROFILE_ME;

	get_sessions_by_coord(ret, coord);
}
void WorldMap::set_player_view(const boost::shared_ptr<PlayerSession> &session, Rectangle view){
	PROFILE_ME;

	const auto map_sector_map = g_map_sector_map.lock();
	if(!map_sector_map){
		LOG_EMPERY_CENTER_WARNING("Map sector map not initialized.");
		DEBUG_THROW(Exception, sslit("Map sector map not initialized"));
	}
	const auto player_view_map = g_player_view_map.lock();
	if(!player_view_map){
		LOG_EMPERY_CENTER_WARNING("Player view map not initialized.");
		DEBUG_THROW(Exception, sslit("Player view map not initialized"));
	}

	player_view_map->erase<0>(session);

	if((view.left() == view.right()) || (view.bottom() == view.top())){
		return;
	}

	const auto sector_bottom_left = get_sector_coord_from_world_coord(Coord(view.left(), view.bottom()));
	const auto sector_upper_right = get_sector_coord_from_world_coord(Coord(view.right() - 1, view.top() - 1));
	LOG_EMPERY_CENTER_DEBUG("Set player view: view = ", view,
		", sector_bottom_left = ", sector_bottom_left, ", sector_upper_right = ", sector_upper_right);
	try {
		for(auto sector_x = sector_bottom_left.x(); sector_x <= sector_upper_right.x(); ++sector_x){
			for(auto sector_y = sector_bottom_left.y(); sector_y <= sector_upper_right.y(); ++sector_y){
				player_view_map->insert(PlayerViewElement(view, session, Coord(sector_x, sector_y)));
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		player_view_map->erase<0>(session);
		throw;
	}
}

void WorldMap::synchronize_player_view(const boost::shared_ptr<PlayerSession> &session, Rectangle view) noexcept {
	PROFILE_ME;

	try {
		boost::container::flat_map<Coord, boost::shared_ptr<MapObject>> map_objects;
		get_map_objects_by_rectangle(map_objects, view);
		for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
			it->second->synchronize_with_client(session);
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		session->shutdown(e.what());
	}
}

Rectangle WorldMap::get_cluster_scope_by_coord(Coord coord){
	PROFILE_ME;

	return Rectangle(get_cluster_coord_from_world_coord(coord), g_map_width, g_map_height);
}

boost::shared_ptr<ClusterSession> WorldMap::get_cluster(Coord coord){
	PROFILE_ME;

	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CENTER_DEBUG("Cluster map not loaded.");
		return { };
	}

	const auto cluster_coord = get_cluster_coord_from_world_coord(coord);
	const auto it = cluster_map->find<0>(cluster_coord);
	if(it == cluster_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Cluster not found: coord = ", coord, ", cluster_coord = ", cluster_coord);
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
void WorldMap::get_all_clusters(std::vector<std::pair<Rectangle, boost::shared_ptr<ClusterSession>>> &ret){
	PROFILE_ME;

	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CENTER_DEBUG("Cluster map not loaded.");
		return;
	}

	ret.reserve(ret.size() + cluster_map->size());
	for(auto it = cluster_map->begin(); it != cluster_map->end(); ++it){
		auto cluster = it->cluster.lock();
		if(!cluster){
			continue;
		}
		ret.emplace_back(it->scope, std::move(cluster));
	}
}
Rectangle WorldMap::get_cluster_scope(const boost::weak_ptr<ClusterSession> &cluster){
	PROFILE_ME;

	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CENTER_WARNING("Cluster map not loaded.");
		return Rectangle(0, 0, 0, 0);
	}

	const auto it = cluster_map->find<1>(cluster);
	if(it == cluster_map->end<1>()){
		LOG_EMPERY_CENTER_DEBUG("Cluster session not found.");
		return Rectangle(0, 0, 0, 0);
	}
	return it->scope;
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
		LOG_EMPERY_CENTER_WARNING("Cluster already registered: old_scope = ", cit->scope);
		DEBUG_THROW(Exception, sslit("Cluster already registered"));
	}

	const auto scope = get_cluster_scope_by_coord(coord);
	LOG_EMPERY_CENTER_INFO("Setting up cluster server: scope = ", scope);
	auto it = cluster_map->find<0>(scope.bottom_left());
	if(it == cluster_map->end<0>()){
		it = cluster_map->insert<0>(it, ClusterElement(scope, cluster));
	} else {
		if(!it->cluster.expired()){
			LOG_EMPERY_CENTER_WARNING("Cluster server conflict:  scope = ", scope);
			DEBUG_THROW(Exception, sslit("Cluster server conflict"));
		}
		cluster_map->set_key<0, 1>(it, cluster);
	}
}
void WorldMap::synchronize_cluster(const boost::shared_ptr<ClusterSession> &cluster, Coord coord) noexcept {
	PROFILE_ME;

	try {
		boost::container::flat_map<Coord, boost::shared_ptr<MapObject>> map_objects;
		get_map_objects_by_rectangle(map_objects, get_cluster_scope_by_coord(coord));
		for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
			notify_cluster_map_object_added(it->second, cluster);
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		cluster->shutdown(e.what());
	}
}

}
