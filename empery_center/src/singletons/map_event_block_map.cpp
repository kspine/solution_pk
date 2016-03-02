#include "../precompiled.hpp"
#include "map_event_block_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/async_job.hpp>
#include "../map_event_block.hpp"
#include "../mysql/map_event.hpp"

namespace EmperyCenter {

namespace {
	enum : unsigned {
		BLOCK_WIDTH  = MapEventBlock::BLOCK_WIDTH,
		BLOCK_HEIGHT = MapEventBlock::BLOCK_HEIGHT,
	};

	struct MapEventBlockElement {
		boost::shared_ptr<MapEventBlock> map_event_block;

		Coord block_coord;

		explicit MapEventBlockElement(boost::shared_ptr<MapEventBlock> map_event_block_)
			: map_event_block(std::move(map_event_block_))
			, block_coord(MapEventBlockMap::get_block_coord_from_world_coord(map_event_block->get_block_coord()))
		{
		}
	};

	MULTI_INDEX_MAP(MapEventBlockContainer, MapEventBlockElement,
		UNIQUE_MEMBER_INDEX(block_coord)
	)

	boost::weak_ptr<MapEventBlockContainer> g_map_event_block_map;

	void refresh_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Item box gc timer: now = ", now);

		const auto map_event_block_map = g_map_event_block_map.lock();
		if(!map_event_block_map){
			return;
		}

		for(auto it = map_event_block_map->begin<0>(); it != map_event_block_map->end<0>(); ++it){
			const auto &map_event_block = it->map_event_block;

			map_event_block->pump_status();
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		struct TempMapEventBlockElement {
			boost::shared_ptr<MySql::Center_MapEventBlock> obj;
			std::vector<boost::shared_ptr<MySql::Center_MapEvent>> events;
		};
		std::map<Coord, TempMapEventBlockElement> temp_map_event_block_map;

		LOG_EMPERY_CENTER_INFO("Loading map event blocks...");
		conn->execute_sql("SELECT * FROM `Center_MapEventBlock`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapEventBlock>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			const auto block_coord = Coord(obj->get_block_x(), obj->get_block_y());
			temp_map_event_block_map[block_coord].obj = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_map_event_block_map.size(), " map event block(s).");

		LOG_EMPERY_CENTER_INFO("Loading map events...");
		conn->execute_sql("SELECT * FROM `Center_MapEvent`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_MapEvent>();
			obj->fetch(conn);
			const auto coord = Coord(obj->get_x(), obj->get_y());
			const auto block_coord = MapEventBlockMap::get_block_coord_from_world_coord(coord);
			const auto it = temp_map_event_block_map.find(block_coord);
			if(it == temp_map_event_block_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.events.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading map events.");

		const auto map_event_block_map = boost::make_shared<MapEventBlockContainer>();
		for(auto it = temp_map_event_block_map.begin(); it != temp_map_event_block_map.end(); ++it){
			auto map_event_block = boost::make_shared<MapEventBlock>(std::move(it->second.obj), it->second.events);

			map_event_block_map->insert(MapEventBlockElement(std::move(map_event_block)));
		}
		g_map_event_block_map = map_event_block_map;
		handles.push(map_event_block_map);

		const auto refresh_interval = get_config<std::uint64_t>("map_event_refresh_interval", 60000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, refresh_interval,
			std::bind(&refresh_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<MapEventBlock> MapEventBlockMap::get(Coord coord){
	PROFILE_ME;

	const auto map_event_block_map = g_map_event_block_map.lock();
	if(!map_event_block_map){
		LOG_EMPERY_CENTER_WARNING("Map event block map not loaded.");
		return { };
	}

	const auto block_coord = MapEventBlockMap::get_block_coord_from_world_coord(coord);
	const auto it = map_event_block_map->find<0>(block_coord);
	if(it == map_event_block_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Map event block not found: coord = ", coord, ", block_coord = ", block_coord);
		return { };
	}
	return it->map_event_block;
}
boost::shared_ptr<MapEventBlock> MapEventBlockMap::require(Coord coord){
	PROFILE_ME;

	auto ret = get(coord);
	if(!ret){
		DEBUG_THROW(Exception, sslit("Map event block map not found"));
	}
	return ret;
}
void MapEventBlockMap::insert(const boost::shared_ptr<MapEventBlock> &map_event_block){
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
void MapEventBlockMap::update(const boost::shared_ptr<MapEventBlock> &map_event_block, bool throws_if_not_exists){
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

void MapEventBlockMap::get_all(std::vector<boost::shared_ptr<MapEventBlock>> &ret){
	PROFILE_ME;

	const auto map_event_block_map = g_map_event_block_map.lock();
	if(!map_event_block_map){
		LOG_EMPERY_CENTER_WARNING("Map event block map not loaded.");
		return;
	}

	ret.reserve(ret.size() + map_event_block_map->size());
	for(auto it = map_event_block_map->begin<0>(); it != map_event_block_map->end<0>(); ++it){
		ret.emplace_back(it->map_event_block);
	}
}

Coord MapEventBlockMap::get_block_coord_from_world_coord(Coord coord){
	const auto mask_x = coord.x() >> 63;
	const auto mask_y = coord.y() >> 63;

	const auto block_x = ((coord.x() ^ mask_x) / BLOCK_WIDTH  ^ mask_x) * BLOCK_WIDTH;
	const auto block_y = ((coord.y() ^ mask_y) / BLOCK_HEIGHT ^ mask_y) * BLOCK_HEIGHT;

	return Coord(block_x, block_y);
}

}
