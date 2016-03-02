#include "precompiled.hpp"
#include "map_event_block.hpp"
#include "mysql/map_event.hpp"
#include "data/map_event.hpp"
#include "data/global.hpp"
#include <poseidon/json.hpp>
#include "singletons/map_event_block_map.hpp"
#include "singletons/world_map.hpp"
#include "strategic_resource.hpp"
#include "map_object.hpp"
#include "map_object_type_ids.hpp"
#include "singletons/account_map.hpp"
#include "account.hpp"
#include "account_attribute_ids.hpp"

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

MapEventBlock::MapEventBlock(Coord block_coord)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_MapEventBlock>(block_coord.x(), block_coord.y(), 0);
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

	const auto utc_now = Poseidon::get_utc_time();
	const auto old_next_refresh_time = m_obj->get_next_refresh_time();
	if(utc_now < old_next_refresh_time){
		return;
	}

	const auto block_coord = get_block_coord();
	const auto cluster_scope = WorldMap::get_cluster_scope(block_coord);
	const auto map_x = static_cast<unsigned>(block_coord.x() - cluster_scope.left());
	const auto map_y = static_cast<unsigned>(block_coord.y() - cluster_scope.bottom());
	const auto event_block_data = Data::MapEventBlock::require(map_x / BLOCK_WIDTH, map_y / BLOCK_HEIGHT);

	const auto refresh_interval = checked_mul<std::uint64_t>(event_block_data->refresh_interval, 60000);

	using BlockDataPtr = boost::shared_ptr<const Data::MapEventBlock>;
	std::vector<BlockDataPtr> event_block_data_all;
	Data::MapEventBlock::get_all(event_block_data_all);
	std::sort(event_block_data_all.begin(), event_block_data_all.end(),
		[](const BlockDataPtr &lhs, const BlockDataPtr &rhs){ return lhs->priority < rhs->priority; });
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
	LOG_EMPERY_CENTER_DEBUG("Refresh map events: block_coord = ", get_block_coord(), ", first_time = ", first_time);

	const auto utc_now = Poseidon::get_utc_time();

	std::vector<boost::shared_ptr<MapObject>> map_objects;

	// 移除过期的地图事件。
	std::vector<Coord> events_to_remove;
	events_to_remove.reserve(m_events.size());
	for(auto it = m_events.begin(); it != m_events.end(); ++it){
		const auto &obj = it->second;
		const auto expiry_time = obj->get_expiry_time();
		if(utc_now < expiry_time){
			continue;
		}
		const auto coord = it->first;
		const auto map_event_id = MapEventId(obj->get_map_event_id());
		// 判定是否应当移除事件。
		const auto resource_event_data = Data::MapEventResource::get(map_event_id);
		if(resource_event_data){
			map_objects.clear();
			WorldMap::get_map_objects_by_rectangle(map_objects, Rectangle(coord, 1, 1));
			if(!map_objects.empty()){
				// 假设有部队正在采集。
				continue;
			}
		}
		events_to_remove.emplace_back(it->first);
	}
	for(auto it = events_to_remove.begin(); it != events_to_remove.end(); ++it){
		const auto coord = *it;
		LOG_EMPERY_CENTER_DEBUG("Removing expired map event: coord = ", coord);
		remove(coord);
	}

	// 刷新新的地图事件。
	const auto block_coord = get_block_coord();
	const auto cluster_scope = WorldMap::get_cluster_scope(block_coord);
	const auto map_x = static_cast<unsigned>(block_coord.x() - cluster_scope.left());
	const auto map_y = static_cast<unsigned>(block_coord.y() - cluster_scope.bottom());
	const auto event_block_data = Data::MapEventBlock::require(map_x / BLOCK_WIDTH, map_y / BLOCK_HEIGHT);

	const auto map_event_circle_id = event_block_data->map_event_circle_id;
	LOG_EMPERY_CENTER_DEBUG("Calculating acitve account count: map_event_circle_id = ", map_event_circle_id);
	char map_event_circle_id_str[32];
	std::sprintf(map_event_circle_id_str, "%lu", (unsigned long)map_event_circle_id.get());
	std::uint64_t active_castle_count = 0;
	if(first_time){
		const auto &init_active_account_object = Data::Global::as_object(Data::Global::SLOT_INIT_ACTIVE_CASTLES_BY_MAP_EVENT_CIRCLE);
		const auto init_active_castle_count = static_cast<std::uint64_t>(
			init_active_account_object.at(SharedNts::view(map_event_circle_id_str)).get<double>());
		active_castle_count = init_active_castle_count;
	} else {
		const auto active_account_threshold_days = Data::Global::as_unsigned(Data::Global::SLOT_ACTIVE_CASTLE_THRESHOLD_DAYS);
		// 统计当前地图上同一圈内的总活跃城堡数。
		std::vector<boost::shared_ptr<MapObject>> map_objects;
		WorldMap::get_map_objects_by_rectangle(map_objects, cluster_scope);
		for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
			const auto &map_object = *it;
			const auto map_object_type_id = map_object->get_map_object_type_id();
			if(map_object_type_id != MapObjectTypeIds::ID_CASTLE){
				continue;
			}
			const auto owner_account = AccountMap::require(map_object->get_owner_uuid());
			std::uint64_t last_logged_out_time = UINT64_MAX;
			const auto &last_logged_out_time_str = owner_account->get_attribute(AccountAttributeIds::ID_LAST_LOGGED_OUT_TIME);
			if(!last_logged_out_time_str.empty()){
				last_logged_out_time = boost::lexical_cast<std::uint64_t>(last_logged_out_time_str);
			}
			LOG_EMPERY_CENTER_TRACE("> Checking active account: account_uuid = ", owner_account->get_account_uuid(),
				", last_logged_out_time = ", last_logged_out_time);
			if(saturated_sub(utc_now, last_logged_out_time) / 86400000 > active_account_threshold_days){
				continue;
			}
			++active_castle_count;
		}

		const auto &min_active_account_object = Data::Global::as_object(Data::Global::SLOT_MIN_ACTIVE_CASTLES_BY_MAP_EVENT_CIRCLE);
		const auto min_active_castle_count = static_cast<std::uint64_t>(
			min_active_account_object.at(SharedNts::view(map_event_circle_id_str)).get<double>());
		if(active_castle_count < min_active_castle_count){
			active_castle_count = min_active_castle_count;
		}
		const auto &max_active_account_object = Data::Global::as_object(Data::Global::SLOT_MAX_ACTIVE_CASTLES_BY_MAP_EVENT_CIRCLE);
		const auto max_active_castle_count = static_cast<std::uint64_t>(
			max_active_account_object.at(SharedNts::view(map_event_circle_id_str)).get<double>());
		if(active_castle_count > max_active_castle_count){
			active_castle_count = max_active_castle_count;
		}
	}
	LOG_EMPERY_CENTER_DEBUG("Number of active castles in map event block: block_coord = ", block_coord,
		", active_castle_count = ", active_castle_count);

	std::vector<boost::shared_ptr<const Data::MapEventBlock>> map_event_block_data_all;
	Data::MapEventBlock::get_all(map_event_block_data_all);
	std::uint32_t block_count = 0;
	for(auto it = map_event_block_data_all.begin(); it != map_event_block_data_all.end(); ++it){
		const auto &map_event_block_data = *it;
		if(map_event_block_data->map_event_circle_id != map_event_circle_id){
			continue;
		}
		++block_count;
	}
	LOG_EMPERY_CENTER_DEBUG("Number of blocks in map event circle: block_count = ", block_count,
		", map_event_circle_id = ", map_event_circle_id);

	
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
