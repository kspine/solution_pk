#include "precompiled.hpp"
#include "map_event_block.hpp"
#include "mysql/map_event.hpp"
#include "data/map_event.hpp"
#include "data/map.hpp"
#include "data/global.hpp"
#include <poseidon/json.hpp>
#include "singletons/map_event_block_map.hpp"
#include "singletons/world_map.hpp"
#include "strategic_resource.hpp"
#include "map_cell.hpp"
#include "map_object.hpp"
#include "map_object_type_ids.hpp"
#include "singletons/account_map.hpp"
#include "account.hpp"
#include "account_attribute_ids.hpp"
#include "events/map.hpp"
#include "map_utilities.hpp"
#include "strategic_resource.hpp"
#include "data/map_object_type.hpp"
#include "attribute_ids.hpp"

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

	void really_create_map_event_at_coord(MapEventId map_event_id,
		Coord coord, std::uint64_t created_time, std::uint64_t expiry_time, Poseidon::Uuid meta_uuid)
	{
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("&& Creating map event: map_event_id = ", map_event_id, ", coord = ", coord);

		const auto event_resource_data = Data::MapEventResource::get(map_event_id);
		if(event_resource_data){
			const auto strategic_resource = boost::make_shared<StrategicResource>(coord,
				event_resource_data->resource_id, event_resource_data->resource_amount, created_time, expiry_time);
			WorldMap::insert_strategic_resource(strategic_resource);
			return;
		}

		const auto event_monster_data = Data::MapEventMonster::get(map_event_id);
		if(event_monster_data){
			const auto monster_data = Data::MapObjectTypeMonster::require(event_monster_data->monster_type_id);
			const auto monster_uuid = MapObjectUuid(meta_uuid);

			boost::container::flat_map<AttributeId, std::int64_t> modifiers;
			modifiers.reserve(8);
			modifiers[AttributeIds::ID_SOLDIER_COUNT]         = static_cast<std::int64_t>(monster_data->max_soldier_count);
			modifiers[AttributeIds::ID_SOLDIER_COUNT_MAX]     = static_cast<std::int64_t>(monster_data->max_soldier_count);
			modifiers[AttributeIds::ID_MONSTER_START_POINT_X] = coord.x();
			modifiers[AttributeIds::ID_MONSTER_START_POINT_Y] = coord.y();

			const auto monster = boost::make_shared<MapObject>(monster_uuid, event_monster_data->monster_type_id,
				AccountUuid(), MapObjectUuid(), std::string(), coord, created_time, false);
			monster->set_attributes(std::move(modifiers));
			monster->pump_status();

			WorldMap::insert_map_object(monster);
			return;
		}

		LOG_EMPERY_CENTER_ERROR("Unknown map event type id: ", map_event_id);
		DEBUG_THROW(Exception, sslit("Unknown map event type id"));
	}
	void really_destroy_map_event_at_coord(MapEventId map_event_id,
		Coord coord, Poseidon::Uuid meta_uuid)
	{
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("&& Creating map event: map_event_id = ", map_event_id, ", coord = ", coord);

		const auto event_resource_data = Data::MapEventResource::get(map_event_id);
		if(event_resource_data){
			const auto strategic_resource = WorldMap::get_strategic_resource(coord);
			if(strategic_resource){
				strategic_resource->delete_from_game();
			}
			return;
		}

		const auto event_monster_data = Data::MapEventMonster::get(map_event_id);
		if(event_monster_data){
			const auto monster_uuid = MapObjectUuid(meta_uuid);
			const auto monster = WorldMap::get_map_object(monster_uuid);
			if(monster){
				monster->delete_from_game();
			}
			return;
		}

		LOG_EMPERY_CENTER_ERROR("Unknown map event type id: ", map_event_id);
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

	boost::container::flat_set<Coord> coords_avail;
	std::vector<boost::shared_ptr<MapCell>> map_cells;
	std::vector<boost::shared_ptr<MapObject>> map_objects;
	std::vector<boost::shared_ptr<StrategicResource>> strategic_resources;
	std::vector<Coord> castle_foundation;

	// 移除过期的地图事件。
	std::deque<Coord> events_to_remove;
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
				const auto &map_object = map_objects.front();
				const auto owner_uuid = map_object->get_owner_uuid();
				if(owner_uuid){
					// 假设有部队正在采集。
					continue;
				}
			}
		}
		events_to_remove.emplace_back(it->first);
	}
	for(auto it = events_to_remove.begin(); it != events_to_remove.end(); ++it){
		const auto coord = *it;
		LOG_EMPERY_CENTER_TRACE("Removing expired map event: coord = ", coord);
		remove(coord);
	}

	// 刷新新的地图事件。
	const auto block_coord = get_block_coord();
	const auto block_scope = Rectangle(block_coord, BLOCK_WIDTH, BLOCK_HEIGHT);
	const auto cluster_scope = WorldMap::get_cluster_scope(block_coord);
	const auto map_x = static_cast<unsigned>(block_coord.x() - cluster_scope.left());
	const auto map_y = static_cast<unsigned>(block_coord.y() - cluster_scope.bottom());
	const auto event_block_data = Data::MapEventBlock::require(map_x / BLOCK_WIDTH, map_y / BLOCK_HEIGHT);

	const auto map_event_circle_id = event_block_data->map_event_circle_id;
	if(!map_event_circle_id){
		LOG_EMPERY_CENTER_DEBUG("Null map event circle id: map_x = ", map_x, ", map_y = ", map_y);
		return;
	}
	LOG_EMPERY_CENTER_DEBUG("Calculating active account count: map_event_circle_id = ", map_event_circle_id);
	char map_event_circle_id_str[32];
	std::sprintf(map_event_circle_id_str, "%lu", (unsigned long)map_event_circle_id.get());
	std::uint64_t active_castle_count = 0;
	if(first_time){
		LOG_EMPERY_CENTER_DEBUG("Performing one-shot initialization: block_scope = ", block_scope);

		const auto &init_active_account_object = Data::Global::as_object(Data::Global::SLOT_INIT_ACTIVE_CASTLES_BY_MAP_EVENT_CIRCLE);
		const auto init_active_castle_count = static_cast<std::uint64_t>(
			init_active_account_object.at(SharedNts::view(map_event_circle_id_str)).get<double>());
		active_castle_count = init_active_castle_count;
	} else {
		const auto active_account_threshold_days = Data::Global::as_unsigned(Data::Global::SLOT_ACTIVE_CASTLE_THRESHOLD_DAYS);
		// 统计当前地图上同一圈内的总活跃城堡数。
		map_objects.clear();
		WorldMap::get_map_objects_by_rectangle(map_objects, cluster_scope);
		for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
			const auto &map_object = *it;
			const auto map_object_type_id = map_object->get_map_object_type_id();
			if(map_object_type_id != MapObjectTypeIds::ID_CASTLE){
				continue;
			}
			const auto coord = map_object->get_coord();
			const auto num_x = static_cast<unsigned>(coord.x() - cluster_scope.left())   / BLOCK_WIDTH;
			const auto num_y = static_cast<unsigned>(coord.y() - cluster_scope.bottom()) / BLOCK_HEIGHT;
			const auto test_block_data = Data::MapEventBlock::get(num_x, num_y);
			if(!test_block_data){
				LOG_EMPERY_CENTER_ERROR("MapEventBlock not found: num_x = ", num_x, ", num_y = ", num_y);
				continue;
			}
			if(test_block_data->map_event_circle_id != map_event_circle_id){
				continue;
			}
			const auto owner_account = AccountMap::require(map_object->get_owner_uuid());
			const auto &last_logged_in_time_str = owner_account->get_attribute(AccountAttributeIds::ID_LAST_LOGGED_IN_TIME);
			if(last_logged_in_time_str.empty()){
				continue;
			}
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
		LOG_EMPERY_CENTER_DEBUG("Active castles: block_scope = ", block_scope, ", active_castle_count = ", active_castle_count);

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
	LOG_EMPERY_CENTER_DEBUG("Virtual active castles: block_scope = ", block_scope, ", active_castle_count = ", active_castle_count);

	const unsigned border_thickness = Data::Global::as_unsigned(Data::Global::SLOT_MAP_BORDER_THICKNESS);

	using GenerationDataPtr = boost::shared_ptr<const Data::MapEventGeneration>;
	std::vector<GenerationDataPtr> generation_data_all;
	Data::MapEventGeneration::get_by_map_event_circle_id(generation_data_all, map_event_circle_id);
	std::sort(generation_data_all.begin(), generation_data_all.end(),
		[](const GenerationDataPtr &lhs, const GenerationDataPtr &rhs){ return lhs->priority < rhs->priority; });
	for(auto gdit = generation_data_all.begin(); gdit != generation_data_all.end(); ++gdit){
		const auto generation_data = *gdit;
		const auto events_to_refresh = static_cast<std::uint64_t>(active_castle_count * generation_data->event_count_multiplier);
		if(events_to_refresh == 0){
			continue;
		}

		const auto map_event_id = generation_data->map_event_id;
		const auto event_data = Data::MapEventAbstract::require(map_event_id);

		try {
			m_events.reserve(events_to_refresh);

			std::uint64_t events_retained = 0;
			for(auto it = m_events.begin(); it != m_events.end(); ++it){
				const auto &obj = it->second;
				const auto old_map_event_id = MapEventId(obj->get_map_event_id());
				if(old_map_event_id != map_event_id){
					continue;
				}
				++events_retained;
			}

			// 删除不能刷事件的点。
			coords_avail.reserve(BLOCK_WIDTH * BLOCK_HEIGHT);
			for(unsigned y = 0; y < BLOCK_HEIGHT; ++y){
				for(unsigned x = 0; x < BLOCK_WIDTH; ++x){
					const auto coord = Coord(block_coord.x() + x, block_coord.y() + y);
					const auto map_x = static_cast<unsigned>(coord.x() - cluster_scope.left());
					const auto map_y = static_cast<unsigned>(coord.y() - cluster_scope.bottom());
					if((map_x < border_thickness) || (map_x >= cluster_scope.width() - border_thickness) ||
						(map_y < border_thickness) || (map_y >= cluster_scope.height() - border_thickness))
					{
						// 事件不能刷在地图边界以外。
						continue;
					}
					coords_avail.emplace_hint(coords_avail.end(), coord);
				}
			}
			map_cells.clear();
			WorldMap::get_map_cells_by_rectangle(map_cells, block_scope);
			for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
				const auto &map_cell = *it;
				const auto coord = map_cell->get_coord();
				if(event_data->restricted_terrain_id){
					const auto map_x = static_cast<unsigned>(coord.x() - cluster_scope.left());
					const auto map_y = static_cast<unsigned>(coord.y() - cluster_scope.bottom());
					const auto basic_data = Data::MapCellBasic::require(map_x, map_y);
					if(basic_data->terrain_id != event_data->restricted_terrain_id){
						// 事件不能刷在非指定类型的土地上。
						coords_avail.erase(coord);
						continue;
					}
				}
			}
			LOG_EMPERY_CENTER_DEBUG("Number of coords statically available: map_event_id = ", map_event_id, ", coords_avail = ", coords_avail.size());
			for(auto it = m_events.begin(); it != m_events.end(); ++it){
				const auto coord = it->first;
				// 事件不能刷在现有事件上。
				coords_avail.erase(coord);
			}
			for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
				const auto &map_cell = *it;
				const auto coord = map_cell->get_coord();
				if(map_cell->get_parent_object_uuid()){
					// 事件不能刷在领地上。
					coords_avail.erase(coord);
					continue;
				}
			}
			map_objects.clear();
			WorldMap::get_map_objects_by_rectangle(map_objects, block_scope);
			for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
				const auto &map_object = *it;
				const auto coord = map_object->get_coord();
				// 事件不能刷在部队上。
				coords_avail.erase(coord);

				const auto map_object_type_id = map_object->get_map_object_type_id();
				if(map_object_type_id == MapObjectTypeIds::ID_CASTLE){
					// 但是对于城堡，考虑其整个范围。
					castle_foundation.clear();
					get_castle_foundation(castle_foundation, coord, true);
					for(auto fit = castle_foundation.begin(); fit != castle_foundation.end(); ++fit){
						coords_avail.erase(*fit);
					}
				}
			}
			strategic_resources.clear();
			WorldMap::get_strategic_resources_by_rectangle(strategic_resources, block_scope);
			for(auto it = strategic_resources.begin(); it != strategic_resources.end(); ++it){
				const auto &strategic_resource = *it;
				const auto coord = strategic_resource->get_coord();
				// 事件不能刷在战略资源上。
				coords_avail.erase(coord);
			}
			LOG_EMPERY_CENTER_DEBUG("About to refresh events: map_event_id = ", map_event_id, ", coords_avail = ", coords_avail.size(),
				", events_retained = ", events_retained, ", events_to_refresh = ", events_to_refresh);

			std::uint64_t events_created = 0;
			for(std::uint64_t i = events_retained; i < events_to_refresh; ++i){
				if(coords_avail.empty()){
					LOG_EMPERY_CENTER_WARNING("No more coords available.");
					break;
				}
				try {
					const auto coord_it = coords_avail.begin() + static_cast<std::ptrdiff_t>(Poseidon::rand32(0, coords_avail.size()));
					const auto coord = *coord_it;

					assert(m_events.find(coord) == m_events.end());

					EventInfo info = { };
					info.coord        = coord;
					info.created_time = utc_now;
					info.expiry_time  = saturated_add(utc_now, saturated_mul<std::uint64_t>(generation_data->expiry_duration, 60000));
					info.map_event_id = map_event_id;
					info.meta_uuid    = Poseidon::Uuid::random();
					insert(std::move(info));

					coords_avail.erase(coord_it);
					++events_created;
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}

			if(events_retained + events_created < events_to_refresh){
				LOG_EMPERY_CENTER_WARNING("Map events overflowed: block_scope = ", block_scope,
					", active_castle_count = ", active_castle_count, ", map_event_id = ", map_event_id,
					", events_to_refresh = ", events_to_refresh, ", events_retained = ", events_retained, ", events_created = ", events_created);
				Poseidon::async_raise_event(
					boost::make_shared<Events::MapEventsOverflowed>(block_scope, active_castle_count, map_event_id,
						events_to_refresh, events_retained, events_created),
					{ });
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}
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
	auto it = m_events.find(coord);
	if(it != m_events.end()){
		LOG_EMPERY_CENTER_WARNING("Map event exists: block_coord = ", get_block_coord(), ", coord = ", coord);
		DEBUG_THROW(Exception, sslit("Map event exists"));
	}

	const auto obj = boost::make_shared<MySql::Center_MapEvent>(coord.x(), coord.y(),
		info.created_time, info.expiry_time, info.map_event_id.get(), info.meta_uuid);
	obj->async_save(true);
	it = m_events.emplace(coord, obj).first;

	try {
		really_create_map_event_at_coord(info.map_event_id, coord, info.created_time, info.expiry_time, info.meta_uuid);
	} catch(...){
		m_events.erase(it);
		throw;
	}
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
	const auto meta_uuid = obj->unlocked_get_meta_uuid();

	really_destroy_map_event_at_coord(map_event_id, coord, meta_uuid);

	return true;
}

}
