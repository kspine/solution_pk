#include "../precompiled.hpp"
#include "world_map.hpp"
#include <boost/container/flat_set.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../map_object.hpp"
#include "../attribute_ids.hpp"
#include "../map_object_type_ids.hpp"
#include "../mysql/map_object.hpp"
#include "../mysql/castle.hpp"
#include "../castle.hpp"
#include "../player_session.hpp"

namespace EmperyCenter {

namespace {
	inline Coord get_sector_coord_from_map_coord(Coord coord){
		assert(coord);

		return Coord(coord.x() >> 5, coord.y() >> 5);
	}

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

	MULTI_INDEX_MAP(WorldMapDelegator, MapObjectElement,
		UNIQUE_MEMBER_INDEX(map_object_uuid)
		MULTI_MEMBER_INDEX(coord)
		MULTI_MEMBER_INDEX(owner_uuid)
		MULTI_MEMBER_INDEX(parent_object_uuid)
	)

	boost::weak_ptr<WorldMapDelegator> g_world_map;

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

	MODULE_RAII_PRIORITY(handles, 5300){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		struct TempMapObjectElement {
			boost::shared_ptr<MySql::Center_MapObject> obj;
			std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> attributes;
		};
		std::map<Poseidon::Uuid, TempMapObjectElement> temp_world_map;

		LOG_EMPERY_CENTER_INFO("Loading map objects...");
		conn->execute_sql("SELECT * FROM `Center_MapObject` WHERE `deleted` = 0");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapObject>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			const auto map_object_uuid = obj->unlocked_get_map_object_uuid();
			temp_world_map[map_object_uuid].obj = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_world_map.size(), " map object(s).");

		LOG_EMPERY_CENTER_INFO("Loading map object attributes...");
		conn->execute_sql("SELECT * FROM `Center_MapObjectAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapObjectAttribute>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			const auto map_object_uuid = obj->unlocked_get_map_object_uuid();
			const auto it = temp_world_map.find(map_object_uuid);
			if(it == temp_world_map.end()){
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
		std::map<Poseidon::Uuid, TempCastleElement> temp_castle_map;

		LOG_EMPERY_CENTER_INFO("Loading castle building bases...");
		conn->execute_sql("SELECT * FROM `Center_CastleBuildingBase`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_CastleBuildingBase>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			const auto map_object_uuid = obj->unlocked_get_map_object_uuid();
			const auto it = temp_world_map.find(map_object_uuid);
			if(it == temp_world_map.end()){
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

		const auto world_map = boost::make_shared<WorldMapDelegator>();
		for(auto it = temp_world_map.begin(); it != temp_world_map.end(); ++it){
			const auto map_object = [&]() -> boost::shared_ptr<MapObject> {
				const auto map_object_type_id = MapObjectTypeId(it->second.obj->get_map_object_type_id());
				if(map_object_type_id == MapObjectTypeIds::ID_CASTLE){
					const auto castle_it = temp_castle_map.find(it->first);
					if(castle_it == temp_castle_map.end()){
						return boost::make_shared<Castle>(std::move(it->second.obj), it->second.attributes);
					}
					return boost::make_shared<Castle>(std::move(it->second.obj), it->second.attributes,
						castle_it->second.buildings, castle_it->second.techs, castle_it->second.resources);
				}
				return boost::make_shared<MapObject>(std::move(it->second.obj), it->second.attributes);
			}();
			world_map->insert(MapObjectElement(std::move(map_object)));
		}
		g_world_map = world_map;
		handles.push(world_map);

		const auto map_sector_map = boost::make_shared<MapSectorMapDelegator>();
		for(auto it = world_map->begin(); it != world_map->end(); ++it){
			auto map_object = it->map_object;

			const auto coord = map_object->get_coord();
			const auto sector_coord = get_sector_coord_from_map_coord(coord);
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
	}

	void synchronize_map_object_by_coord(const boost::shared_ptr<MapObject> &map_object, Coord coord) noexcept {
		PROFILE_ME;

		const auto player_view_map = g_player_view_map.lock();
		if(!player_view_map){
			LOG_EMPERY_CENTER_WARNING("Player view map is not initialized.");
			return;
		}

		const auto sector_coord = get_sector_coord_from_map_coord(coord);
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
					map_object->synchronize_with_client(session);
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
					session->shutdown(e.what());
				}
			}
			++view_it;
		}
	}
	void on_update(const boost::shared_ptr<MapObject> &map_object, Coord old_coord, Coord new_coord) noexcept {
		PROFILE_ME;

		if(old_coord && (old_coord != new_coord)){
			synchronize_map_object_by_coord(map_object, old_coord);
		}
		if(new_coord){
			synchronize_map_object_by_coord(map_object, new_coord);
		}
	}
}

boost::shared_ptr<MapObject> WorldMap::get_map_object(MapObjectUuid map_object_uuid){
	PROFILE_ME;

	const auto world_map = g_world_map.lock();
	if(!world_map){
		LOG_EMPERY_CENTER_WARNING("Map object map is not loaded.");
		return { };
	}

	const auto it = world_map->find<0>(map_object_uuid);
	if(it == world_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Map object is not found: map_object_uuid = ", map_object_uuid);
		return { };
	}
	return it->map_object;
}
void WorldMap::insert_map_object(const boost::shared_ptr<MapObject> &map_object){
	PROFILE_ME;

	const auto world_map = g_world_map.lock();
	if(!world_map){
		LOG_EMPERY_CENTER_WARNING("Map object map is not loaded.");
		DEBUG_THROW(Exception, sslit("Map object map is not loaded"));
	}
	const auto map_sector_map = g_map_sector_map.lock();
	if(!map_sector_map){
		LOG_EMPERY_CENTER_WARNING("Map sector map is not loaded.");
		DEBUG_THROW(Exception, sslit("Map sector map is not loaded"));
	}

	const auto map_object_uuid = map_object->get_map_object_uuid();

	if(map_object->has_been_deleted()){
		LOG_EMPERY_CENTER_WARNING("Map object has been marked as deleted: map_object_uuid = ", map_object_uuid);
		DEBUG_THROW(Exception, sslit("Map object has been marked as deleted"));
	}

	const auto it = world_map->find<0>(map_object_uuid);
	if(it != world_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Map object already exists: map_object_uuid = ", map_object_uuid);
		DEBUG_THROW(Exception, sslit("Map object already exists"));
	}

	const auto new_coord        = map_object->get_coord();
	const auto new_sector_coord = get_sector_coord_from_map_coord(new_coord);
	const auto result = map_sector_map->insert<0>(MapSectorElement(new_sector_coord));
	if(result.second){
		LOG_EMPERY_CENTER_DEBUG("Created map sector: new_sector_coord = ", new_sector_coord);
	}
	const auto new_sector_it    = result.first;
	new_sector_it->map_objects.reserve(new_sector_it->map_objects.size() + 1);

	LOG_EMPERY_CENTER_DEBUG("Inserting map object: map_object_uuid = ", map_object_uuid,
		", new_coord = ", new_coord, ", new_sector_coord = ", new_sector_coord);
	world_map->insert(MapObjectElement(map_object));
	new_sector_it->map_objects.insert(map_object); // 确保事先 reserve() 过。

	on_update(map_object, { }, new_coord);
}
void WorldMap::update_map_object(const boost::shared_ptr<MapObject> &map_object, bool throws_if_not_exists){
	PROFILE_ME;

	const auto world_map = g_world_map.lock();
	if(!world_map){
		LOG_EMPERY_CENTER_WARNING("Map object map is not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map object map is not loaded"));
		}
		return;
	}
	const auto map_sector_map = g_map_sector_map.lock();
	if(!map_sector_map){
		LOG_EMPERY_CENTER_WARNING("Map sector map is not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map sector map is not loaded"));
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

	const auto it = world_map->find<0>(map_object_uuid);
	if(it == world_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Map object is not found: map_object_uuid = ", map_object_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map object is not found"));
		}
		return;
	}
	const auto old_coord        = it->coord;
	const auto old_sector_coord = get_sector_coord_from_map_coord(old_coord);
	const auto old_sector_it    = map_sector_map->find<0>(old_sector_coord);

	const auto new_coord        = map_object->get_coord();
	const auto new_sector_coord = get_sector_coord_from_map_coord(new_coord);
	const auto result = map_sector_map->insert<0>(MapSectorElement(new_sector_coord));
	if(result.second){
		LOG_EMPERY_CENTER_DEBUG("Created map sector: new_sector_coord = ", new_sector_coord);
	}
	const auto new_sector_it    = result.first;
	new_sector_it->map_objects.reserve(new_sector_it->map_objects.size() + 1);

	LOG_EMPERY_CENTER_DEBUG("Updating map object: map_object_uuid = ", map_object_uuid,
		", old_coord = ", old_coord, ", old_sector_coord = ", old_sector_coord,
		", new_coord = ", new_coord, ", new_sector_coord = ", new_sector_coord);
	world_map->set_key<0, 1>(it, new_coord);
	if(old_sector_it != map_sector_map->end<0>()){
		old_sector_it->map_objects.erase(map_object); // noexcept
		if(old_sector_it->map_objects.empty()){
			map_sector_map->erase<0>(old_sector_it);
			LOG_EMPERY_CENTER_DEBUG("Removed map sector: old_sector_coord = ", old_sector_coord);
		}
	}
	new_sector_it->map_objects.insert(map_object); // 确保事先 reserve() 过。

	on_update(map_object, old_coord, new_coord);
}
void WorldMap::remove_map_object(MapObjectUuid map_object_uuid) noexcept {
	PROFILE_ME;

	const auto world_map = g_world_map.lock();
	if(!world_map){
		LOG_EMPERY_CENTER_WARNING("Map object map is not loaded.");
		return;
	}
	const auto map_sector_map = g_map_sector_map.lock();
	if(!map_sector_map){
		LOG_EMPERY_CENTER_FATAL("Map sector map is not loaded.");
		std::abort();
	}

	const auto it = world_map->find<0>(map_object_uuid);
	if(it == world_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Map object is not found: map_object_uuid = ", map_object_uuid);
		return;
	}
	const auto map_object       = it->map_object;

	const auto old_coord        = it->coord;
	const auto old_sector_coord = get_sector_coord_from_map_coord(old_coord);
	const auto old_sector_it    = map_sector_map->find<0>(old_sector_coord);

	LOG_EMPERY_CENTER_DEBUG("Removing map object: map_object_uuid = ", map_object_uuid,
		", old_coord = ", old_coord, ", old_sector_coord = ", old_sector_coord);
	world_map->erase<0>(it);
	if(old_sector_it != map_sector_map->end<0>()){
		old_sector_it->map_objects.erase(map_object); // noexcept
		if(old_sector_it->map_objects.empty()){
			map_sector_map->erase<0>(old_sector_it);
			LOG_EMPERY_CENTER_DEBUG("Removed map sector: old_sector_coord = ", old_sector_coord);
		}
	}

	on_update(map_object, old_coord, { });
}

void WorldMap::get_map_objects_by_owner(std::vector<boost::shared_ptr<MapObject>> &ret, AccountUuid owner_uuid){
	PROFILE_ME;

	const auto world_map = g_world_map.lock();
	if(!world_map){
		LOG_EMPERY_CENTER_WARNING("Map object map is not loaded.");
		return;
	}

	const auto range = world_map->equal_range<2>(owner_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->map_object);
	}
}
void WorldMap::get_map_objects_by_parent_object(std::vector<boost::shared_ptr<MapObject>> &ret, MapObjectUuid parent_object_uuid){
	PROFILE_ME;

	const auto world_map = g_world_map.lock();
	if(!world_map){
		LOG_EMPERY_CENTER_WARNING("Map object map is not loaded.");
		return;
	}

	const auto range = world_map->equal_range<3>(parent_object_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->map_object);
	}
}
void WorldMap::get_map_objects_by_rectangle(std::vector<boost::shared_ptr<MapObject>> &ret, Rectangle rectangle){
	PROFILE_ME;

	const auto world_map = g_world_map.lock();
	if(!world_map){
		LOG_EMPERY_CENTER_WARNING("Map object map is not loaded.");
		return;
	}

	auto x = rectangle.left();
	while(x < rectangle.right()){
		auto it = world_map->lower_bound<1>(Coord(x, rectangle.bottom()));
		for(;;){
			if(it == world_map->end<1>()){
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

void WorldMap::set_player_view(const boost::shared_ptr<PlayerSession> &session, Rectangle view){
	PROFILE_ME;

	const auto map_sector_map = g_map_sector_map.lock();
	if(!map_sector_map){
		LOG_EMPERY_CENTER_WARNING("Map sector map is not initialized.");
		DEBUG_THROW(Exception, sslit("Map sector map is not initialized"));
	}
	const auto player_view_map = g_player_view_map.lock();
	if(!player_view_map){
		LOG_EMPERY_CENTER_WARNING("Player view map is not initialized.");
		DEBUG_THROW(Exception, sslit("Player view map is not initialized"));
	}

	player_view_map->erase<0>(session);

	if((view.left() == view.right()) || (view.bottom() == view.top())){
		return;
	}

	const auto sector_bottom_left = get_sector_coord_from_map_coord(Coord(view.left(), view.bottom()));
	const auto sector_upper_right = get_sector_coord_from_map_coord(Coord(view.right() - 1, view.top() - 1));
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

void WorldMap::synchronize_player_view(const boost::shared_ptr<PlayerSession> &session, Rectangle view) noexcept
try {
	PROFILE_ME;

	std::vector<boost::shared_ptr<MapObject>> map_objects;
	get_map_objects_by_rectangle(map_objects, view);
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = (*it);
		map_object->synchronize_with_client(session);
	}
} catch(std::exception &e){
	LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	session->shutdown(e.what());
}

}