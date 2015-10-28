#include "../precompiled.hpp"
#include "map_object_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../map_object.hpp"
#include "../map_object_attr_ids.hpp"
#include "../mysql/map_object.hpp"
#include "../mysql/map_object_attribute.hpp"

namespace EmperyCenter {

namespace {
	struct MapObjectElement {
		boost::shared_ptr<MapObject> mapObject;

		MapObjectUuid mapObjectUuid;
		Coord coord;

		MapObjectElement(boost::shared_ptr<MapObject> mapObject_, const Coord &coord_)
			: mapObject(std::move(mapObject_))
			, mapObjectUuid(mapObject->getMapObjectUuid()), coord(coord_)
		{
		}
	};

	MULTI_INDEX_MAP(MapObjectMapDelegator, MapObjectElement,
		UNIQUE_MEMBER_INDEX(mapObjectUuid)
		MULTI_MEMBER_INDEX(coord)
	)

	boost::weak_ptr<MapObjectMapDelegator> g_mapObjectMap;

	MODULE_RAII_PRIORITY(handles, 5300){
		const auto conn = Poseidon::MySqlDaemon::createConnection();

		std::map<Poseidon::Uuid, std::pair<
			boost::shared_ptr<MySql::Center_MapObject>,
			std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>>
			>> tempMap;

		LOG_EMPERY_CENTER_INFO("Loading map objects...");
		conn->executeSql("SELECT * FROM `Center_MapObject`");
		while(conn->fetchRow()){
			auto obj = boost::make_shared<MySql::Center_MapObject>();
			obj->syncFetch(conn);
			obj->enableAutoSaving();
			const auto mapObjectUuid = obj->unlockedGet_mapObjectUuid();
			if(obj->get_deleted()){
				LOG_EMPERY_CENTER_WARNING("The following map object has been marked as deleted, ",
					"consider deleting it from database: mapObjectUuid = ", mapObjectUuid);
				continue;
			}
			auto &pair = tempMap[mapObjectUuid];
			pair.first = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", tempMap.size(), " map object(s).");

		LOG_EMPERY_CENTER_INFO("Loading map object attributes...");
		conn->executeSql("SELECT * FROM `Center_MapObjectAttribute`");
		while(conn->fetchRow()){
			auto obj = boost::make_shared<MySql::Center_MapObjectAttribute>();
			obj->syncFetch(conn);
			obj->enableAutoSaving();
			const auto mapObjectUuid = obj->unlockedGet_mapObjectUuid();
			const auto it = tempMap.find(mapObjectUuid);
			if(it == tempMap.end()){
				LOG_EMPERY_CENTER_WARNING("A dangling map object attribute has been found, ",
					"consider deleting it from database: mapObjectUuid = ", mapObjectUuid);
				continue;
			}
			it->second.second.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", tempMap.size(), " map object attribute(s).");

		const auto mapObjectMap = boost::make_shared<MapObjectMapDelegator>();
		for(auto it = tempMap.begin(); it != tempMap.end(); ++it){
			auto mapObject = boost::make_shared<MapObject>(std::move(it->second.first), it->second.second);
			auto coord = Coord(mapObject->getAttribute(MapObjectAttrIds::ID_COORD_X),
				mapObject->getAttribute(MapObjectAttrIds::ID_COORD_Y));
			mapObjectMap->insert(MapObjectElement(std::move(mapObject), coord));
		}
		g_mapObjectMap = mapObjectMap;
		handles.push(mapObjectMap);
	}
}

boost::shared_ptr<MapObject> MapObjectMap::get(const MapObjectUuid &mapObjectUuid){
	PROFILE_ME;

	const auto mapObjectMap = g_mapObjectMap.lock();
	if(!mapObjectMap){
		LOG_EMPERY_CENTER_WARNING("Map object map is not loaded.");
		return { };
	}

	const auto it = mapObjectMap->find<0>(mapObjectUuid);
	if(it == mapObjectMap->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Map object is not found: mapObjectUuid = ", mapObjectUuid);
		return { };
	}
	return it->mapObject;
}
void MapObjectMap::update(const boost::shared_ptr<MapObject> &mapObject, const Coord &coord){
	PROFILE_ME;

	const auto mapObjectMap = g_mapObjectMap.lock();
	if(!mapObjectMap){
		LOG_EMPERY_CENTER_WARNING("Map object map is not loaded.");
		DEBUG_THROW(Exception, sslit("Map object map is not loaded"));
	}

	const auto mapObjectUuid = mapObject->getMapObjectUuid();

	if(mapObject->hasBeenDeleted()){
		LOG_EMPERY_CENTER_DEBUG("Map object has been marked as deleted: mapObjectUuid = ", mapObjectUuid);
		remove(mapObjectUuid);
		return;
	}

	auto it = mapObjectMap->find<0>(mapObjectUuid);
	if(it == mapObjectMap->end<0>()){
		it = mapObjectMap->insert<0>(it, MapObjectElement(mapObject, coord));
	} else {
		mapObjectMap->setKey<0, 1>(it, coord);
	}
	it->mapObject->setAttribute(MapObjectAttrIds::ID_COORD_X, coord.x);
	it->mapObject->setAttribute(MapObjectAttrIds::ID_COORD_Y, coord.y);
}
void MapObjectMap::remove(const MapObjectUuid &mapObjectUuid) noexcept {
	PROFILE_ME;

	const auto mapObjectMap = g_mapObjectMap.lock();
	if(!mapObjectMap){
		LOG_EMPERY_CENTER_WARNING("Map object map is not loaded.");
		return;
	}

	const auto it = mapObjectMap->find<0>(mapObjectUuid);
	if(it == mapObjectMap->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Map object is not found: mapObjectUuid = ", mapObjectUuid);
		return;
	}
	const auto mapObject = it->mapObject;
	mapObjectMap->erase<0>(it);

	try {
		mapObject->deleteFromGame();
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}
}

void MapObjectMap::getByRectangle(std::vector<std::pair<Coord, boost::shared_ptr<MapObject>>> &ret, const Rectangle &rectangle){
	PROFILE_ME;

	const auto mapObjectMap = g_mapObjectMap.lock();
	if(!mapObjectMap){
		LOG_EMPERY_CENTER_WARNING("Map object map is not loaded.");
		return;
	}

	auto x = rectangle.left();
	while(x < rectangle.right()){
		auto it = mapObjectMap->lowerBound<1>(Coord(x, rectangle.bottom()));
		for(;;){
			if(it == mapObjectMap->end<1>()){
				x = INT64_MAX;
				break;
			}
			if(it->coord.x != x){
				x = it->coord.x;
				break;
			}
			if(it->coord.y >= rectangle.top()){
				++x;
				break;
			}
			ret.emplace_back(it->coord, it->mapObject);
			++it;
		}
	}
}

}
