#include "precompiled.hpp"
#include "map_event_block.hpp"
#include "mysql/map_event.hpp"
#include "data/map_event.hpp"
#include "data/global.hpp"
#include "singletons/world_map.hpp"
#include "strategic_resource.hpp"
#include "map_object.hpp"

namespace EmperyCenter {

namespace {
	void fill_map_event_info(MapEventBlock::EventInfo &info, const boost::shared_ptr<MySql::Center_MapEvent> &obj){
		PROFILE_ME;

		info.coord        = Coord(obj->get_x(), obj->get_y());
		info.created_time = obj->get_created_time();
		info.expiry_time  = obj->get_expiry_time();
		info.map_event_id = MapEventId(obj->get_map_event_id());
		info.meta_uuid    = obj->get_meta_uuid();
	}
}

MapEventBlock::MapEventBlock(Coord coord)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_MapEventBlock>(coord.x(), coord.y(), 0);
			obj->async_save(true);
			return obj;
		}())
{
}
MapEventBlock::MapEventBlock(boost::shared_ptr<MySql::Center_MapEventBlock> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapEvent>> &events)
	: m_obj(std::move(obj))
{
	for(auto it = events.begin(); it != events.end(); ++it){
		const auto coord = Coord((*it)->get_x(), (*it)->get_y());
		m_events.emplace(coord, *it);
	}
}
MapEventBlock::~MapEventBlock(){
}

void MapEventBlock::pump_status(){
	PROFILE_ME;

	const auto block_coord = get_block_coord();
	const auto cluster_scope = WorldMap::get_cluster_scope(block_coord);
	const auto map_x = static_cast<unsigned>(block_coord.x() - cluster_scope.left());
	const auto map_y = static_cast<unsigned>(block_coord.y() - cluster_scope.bottom());
	const auto event_block_data = Data::MapEventBlock::require(map_x / BLOCK_WIDTH, map_y / BLOCK_HEIGHT);

	const auto utc_now = Poseidon::get_utc_time();

	const auto old_next_refresh_time = m_obj->get_next_refresh_time();
	if(utc_now < old_next_refresh_time){
		return;
	}

	const auto refresh_interval = checked_mul<std::uint64_t>(event_block_data->refresh_interval, 60000);

	std::vector<boost::shared_ptr<const Data::MapEventBlock>> event_block_data_all;
	Data::MapEventBlock::get_all(event_block_data_all);
	using DataPtr = boost::shared_ptr<const Data::MapEventBlock>;
	std::sort(event_block_data_all.begin(), event_block_data_all.end(),
		[](const DataPtr &lhs, const DataPtr &rhs){ return lhs->priority < rhs->priority; });
	const auto index_by_priority = static_cast<std::size_t>(
		std::find(event_block_data_all.begin(), event_block_data_all.end(), event_block_data) - event_block_data_all.begin());
	if(index_by_priority >= event_block_data_all.size()){
		DEBUG_THROW(Exception, sslit("MapEventBlock came out of thin air?"));
	}
	const auto refresh_offset = (86400000 / event_block_data_all.size() * index_by_priority) % refresh_interval;

	refresh_events(old_next_refresh_time == 0);

	const auto interval_index = saturated_sub(utc_now, refresh_offset) / refresh_interval;
	const auto new_next_refresh_time = saturated_add((interval_index + 1) * refresh_interval, refresh_offset);
	m_obj->set_next_refresh_time(new_next_refresh_time);
}

void MapEventBlock::refresh_events(bool first_time){
	PROFILE_ME;

	
}

Coord MapEventBlock::get_block_coord() const {
	return Coord(m_obj->get_block_x(), m_obj->get_block_y());
}
std::uint64_t MapEventBlock::get_next_refresh_time() const {
	return m_obj->get_next_refresh_time();
}

MapEventBlock::EventInfo MapEventBlock::get(Coord coord) const {
	PROFILE_ME;

	EventInfo info = { };
	info.coord = coord;

	const auto it = m_events.find(coord);
	if(it == m_events.end()){
		return info;
	}
	fill_map_event_info(info, it->second);
	return info;
}
void MapEventBlock::get_all(std::vector<MapEventBlock::EventInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_events.size());
	for(auto it = m_events.begin(); it != m_events.end(); ++it){
		EventInfo info;
		fill_map_event_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

void MapEventBlock::insert(MapEventBlock::EventInfo info){
	PROFILE_ME;

	const auto coord = info.coord;
	const auto it = m_events.find(coord);
	if(it != m_events.end()){
		LOG_EMPERY_CENTER_WARNING("Map event exists: block_coord = ", get_block_coord(), ", coord = ", coord);
		DEBUG_THROW(Exception, sslit("Map event exists"));
	}

	const auto obj = boost::make_shared<MySql::Center_MapEvent>(coord.x(), coord.y(),
		info.created_time, info.expiry_time, info.map_event_id.get(), info.meta_uuid);
	obj->async_save(true);
	m_events.emplace(coord, obj);
}
void MapEventBlock::update(EventInfo info, bool throws_if_not_exists){
	PROFILE_ME;

	const auto coord = info.coord;
	const auto it = m_events.find(coord);
	if(it == m_events.end()){
		LOG_EMPERY_CENTER_WARNING("Map event not found: block_coord = ", get_block_coord(), ", coord = ", coord);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map event not found"));
		}
		return;
	}
	const auto &obj = it->second;

	obj->set_expiry_time(info.expiry_time);
	obj->set_map_event_id(info.map_event_id.get());
	obj->set_meta_uuid(info.meta_uuid);
}
bool MapEventBlock::remove(Coord coord) noexcept {
	PROFILE_ME;

	const auto it = m_events.find(coord);
	if(it == m_events.end()){
		return false;
	}
	const auto obj = std::move(it->second);
	m_events.erase(it);

	obj->set_expiry_time(0);

	const auto map_event_id = MapEventId(obj->get_map_event_id());

	const auto resource_event_data = Data::MapEventResource::get(map_event_id);
	if(resource_event_data){
		const auto strategic_resource = WorldMap::get_strategic_resource(coord);
		if(strategic_resource){
			strategic_resource->delete_from_game();
		}
	}

	const auto monster_event_data = Data::MapEventMonster::get(map_event_id);
	if(monster_event_data){
		const auto monster_uuid = MapObjectUuid(obj->unlocked_get_meta_uuid());
		const auto monster = WorldMap::get_map_object(monster_uuid);
		if(monster){
			monster->delete_from_game();
		}
	}

	return true;
}

}
