#include "../precompiled.hpp"
#include "world_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../castle.hpp"
#include "../../../empery_center/src/mysql/map_object.hpp"
#include "../../../empery_center/src/map_object_type_ids.hpp"

namespace EmperyController {

namespace {
	struct CastleElement {
		boost::shared_ptr<Castle> castle;

		MapObjectUuid map_object_uuid;
		AccountUuid owner_uuid;
		MapObjectUuid parent_object_uuid;

		explicit CastleElement(boost::shared_ptr<Castle> castle_)
			: castle(std::move(castle_))
			, map_object_uuid(castle->get_map_object_uuid())
			, owner_uuid(castle->get_owner_uuid()), parent_object_uuid(castle->get_parent_object_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(CastleContainer, CastleElement,
		UNIQUE_MEMBER_INDEX(map_object_uuid)
		MULTI_MEMBER_INDEX(owner_uuid)
		MULTI_MEMBER_INDEX(parent_object_uuid)
	)

	boost::weak_ptr<CastleContainer> g_castle_map;

	MODULE_RAII_PRIORITY(handles, 5300){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto castle_map = boost::make_shared<CastleContainer>();
		LOG_EMPERY_CONTROLLER_INFO("Loading castles...");
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_MapObject` WHERE `deleted` = 0 AND `map_object_type_id` = " <<EmperyCenter::MapObjectTypeIds::ID_CASTLE;
		conn->execute_sql(oss.str());
		while(conn->fetch_row()){
			auto obj = boost::make_shared<EmperyCenter::MySql::Center_MapObject>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			auto castle = boost::make_shared<Castle>(std::move(obj));
			castle_map->insert(CastleElement(std::move(castle)));
		}
		LOG_EMPERY_CONTROLLER_INFO("Loaded ", castle_map->size(), " castle(s).");
		g_castle_map = castle_map;
		handles.push(castle_map);
	}
}

boost::shared_ptr<Castle> WorldMap::get_castle(MapObjectUuid map_object_uuid){
	PROFILE_ME;

	const auto castle_map = g_castle_map.lock();
	if(!castle_map){
		LOG_EMPERY_CONTROLLER_WARNING("Castle map not loaded.");
		return { };
	}

	const auto it = castle_map->find<0>(map_object_uuid);
	if(it == castle_map->end<0>()){
		LOG_EMPERY_CONTROLLER_TRACE("Castle not found: map_object_uuid = ", map_object_uuid);
		return { };
	}
	return it->castle;
}
boost::shared_ptr<Castle> WorldMap::require_castle(MapObjectUuid map_object_uuid){
	PROFILE_ME;

	auto castle = get_castle(map_object_uuid);
	if(!castle){
		LOG_EMPERY_CONTROLLER_WARNING("Castle not found: map_object_uuid = ", map_object_uuid);
		DEBUG_THROW(Exception, sslit("Castle not found"));
	}
	return castle;
}
boost::shared_ptr<Castle> WorldMap::get_or_reload_castle(MapObjectUuid map_object_uuid){
	PROFILE_ME;

	auto castle = get_castle(map_object_uuid);
	if(!castle){
		castle = forced_reload_castle(map_object_uuid);
	}
	return castle;
}
boost::shared_ptr<Castle> WorldMap::forced_reload_castle(MapObjectUuid map_object_uuid){
	PROFILE_ME;
	LOG_EMPERY_CONTROLLER_DEBUG("Reloading account: map_object_uuid = ", map_object_uuid);

	const auto castle_map = g_castle_map.lock();
	if(!castle_map){
		LOG_EMPERY_CONTROLLER_WARNING("Castle map not loaded.");
		return { };
	}

	std::vector<boost::shared_ptr<EmperyCenter::MySql::Center_MapObject>> sink;
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
		LOG_EMPERY_CONTROLLER_DEBUG("Castle not found in database: map_object_uuid = ", map_object_uuid);
		castle_map->erase<0>(map_object_uuid);
		return { };
	}

	auto castle = boost::make_shared<Castle>(std::move(sink.front()));

	auto it = castle_map->find<0>(map_object_uuid);
	if(it == castle_map->end<0>()){\
		it = castle_map->insert<0>(CastleElement(castle)).first;
	} else {
		castle_map->replace<0>(it, CastleElement(castle));
	}

	LOG_EMPERY_CONTROLLER_DEBUG("Successfully reloaded castle: map_object_uuid = ", map_object_uuid);
	return std::move(castle);
}

void WorldMap::get_castles_by_owner(std::vector<boost::shared_ptr<Castle>> &ret, AccountUuid owner_uuid){
	PROFILE_ME;

	const auto castle_map = g_castle_map.lock();
	if(!castle_map){
		LOG_EMPERY_CONTROLLER_WARNING("Castle map not loaded.");
		return;
	}

	const auto range = castle_map->equal_range<1>(owner_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->castle);
	}
}
void WorldMap::get_castles_by_parent_object(std::vector<boost::shared_ptr<Castle>> &ret, MapObjectUuid parent_object_uuid){
	PROFILE_ME;

	const auto castle_map = g_castle_map.lock();
	if(!castle_map){
		LOG_EMPERY_CONTROLLER_WARNING("Castle map not loaded.");
		return;
	}

	const auto range = castle_map->equal_range<2>(parent_object_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->castle);
	}
}

}
