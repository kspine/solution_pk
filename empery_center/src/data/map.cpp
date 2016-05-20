#include "../precompiled.hpp"
#include "map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "../map_object_type_ids.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(BasicMap, Data::MapCellBasic,
		UNIQUE_MEMBER_INDEX(map_coord)
		MULTI_MEMBER_INDEX(overlay_group_name)
	)
	boost::weak_ptr<const BasicMap> g_basic_map;
	const char BASIC_FILE[] = "map";

	MULTI_INDEX_MAP(TicketMap, Data::MapCellTicket,
		UNIQUE_MEMBER_INDEX(ticket_item_id)
	)
	boost::weak_ptr<const TicketMap> g_ticket_map;
	const char TICKET_FILE[] = "Territory_levelup";

	MULTI_INDEX_MAP(TerrainMap, Data::MapTerrain,
		UNIQUE_MEMBER_INDEX(terrain_id)
	)
	boost::weak_ptr<const TerrainMap> g_terrain_map;
	const char TERRAIN_FILE[] = "Territory_product";

	MULTI_INDEX_MAP(OverlayMap, Data::MapOverlay,
		UNIQUE_MEMBER_INDEX(overlay_id)
	)
	boost::weak_ptr<const OverlayMap> g_overlay_map;
	const char OVERLAY_FILE[] = "remove";

	MULTI_INDEX_MAP(StartPointMap, Data::MapStartPoint,
		UNIQUE_MEMBER_INDEX(start_point_id)
		UNIQUE_MEMBER_INDEX(map_coord)
	)
	boost::weak_ptr<const StartPointMap> g_start_point_map;
	const char START_POINT_FILE[] = "birth_point";

	MULTI_INDEX_MAP(CrateMap, Data::MapCrate,
		UNIQUE_MEMBER_INDEX(crate_id)
		UNIQUE_MEMBER_INDEX(resource_amount_key)
	)
	boost::weak_ptr<const CrateMap> g_crate_map;
	const char CRATE_FILE[] = "chest";

	using CastleMap = boost::container::flat_map<unsigned, Data::MapDefenseBuildingCastle>;
	boost::weak_ptr<const CastleMap> g_castle_map;
	const char CASTLE_FILE[] = "Building_castel";

	using DefenseTowerMap = boost::container::flat_map<unsigned, Data::MapDefenseBuildingDefenseTower>;
	boost::weak_ptr<const DefenseTowerMap> g_defense_tower_map;
	const char DEFENSE_TOWER_FILE[] = "Building_towers";

	using BattleBunkerMap = boost::container::flat_map<unsigned, Data::MapDefenseBuildingBattleBunker>;
	boost::weak_ptr<const BattleBunkerMap> g_battle_bunker_map;
	const char BATTLE_BUNKER_FILE[] = "Building_bunker";

	MULTI_INDEX_MAP(DefenseCombatMap, Data::MapDefenseCombat,
		UNIQUE_MEMBER_INDEX(defense_combat_id)
	)
	boost::weak_ptr<const DefenseCombatMap> g_defense_combat_map;
	const char DEFENSE_COMBAT_FILE[] = "Building_combat_attributes";

	template<typename ElementT>
	void read_defense_building_abstract(ElementT &elem, const Poseidon::CsvParser &csv){
		csv.get(elem.upgrade_duration,       "building_upgrade_time");

		Poseidon::JsonObject object;
		csv.get(object, "building_upgrade_cost");
		elem.upgrade_cost.reserve(object.size());
		for(auto it = object.begin(); it != object.end(); ++it){
			const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
			const auto resource_amount = it->second.get<double>();
			if(!elem.upgrade_cost.emplace(resource_id, resource_amount).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade resource cost: resource_id = ", resource_id);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade resource cost"));
			}
		}

		object.clear();
		csv.get(object, "building_upgrade_condition");
		elem.prerequisite.reserve(object.size());
		for(auto it = object.begin(); it != object.end(); ++it){
			const auto building_id = boost::lexical_cast<BuildingId>(it->first);
			const auto building_level = static_cast<unsigned>(it->second.get<double>());
			if(!elem.prerequisite.emplace(building_id, building_level).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade prerequisite: building_id = ", building_id);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade prerequisite"));
			}
		}

		object.clear();
		csv.get(object, "material_get");
		elem.debris.reserve(object.size());
		for(auto it = object.begin(); it != object.end(); ++it){
			const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
			const auto resource_amount = it->second.get<double>();
			if(!elem.debris.emplace(resource_id, resource_amount).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade resource cost: resource_id = ", resource_id);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade resource cost"));
			}
		}

		csv.get(elem.destruct_duration,      "building_dismantling_time");
		csv.get(elem.defense_combat_id,      "building_combat_attributes");
	}

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(BASIC_FILE);
		const auto basic_map = boost::make_shared<BasicMap>();
		while(csv.fetch_row()){
			Data::MapCellBasic elem = { };

			Poseidon::JsonArray array;
			csv.get(array, "xy");
			const unsigned x = array.at(0).get<double>();
			const unsigned y = array.at(1).get<double>();
			elem.map_coord = std::make_pair(x, y);

			csv.get(elem.terrain_id,         "property_id");
			csv.get(elem.overlay_id,         "remove_id");
			csv.get(elem.overlay_group_name, "group_id");

			if(!basic_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapCellBasic: x = ", elem.map_coord.first, ", y = ", elem.map_coord.second);
				DEBUG_THROW(Exception, sslit("Duplicate MapCellBasic"));
			}
		}
		g_basic_map = basic_map;
		handles.push(basic_map);
		// servlet = DataSession::create_servlet(BASIC_FILE, Data::encode_csv_as_json(csv, "xy"));
		// handles.push(std::move(servlet));

		csv = Data::sync_load_data(TICKET_FILE);
		const auto ticket_map = boost::make_shared<TicketMap>();
		while(csv.fetch_row()){
			Data::MapCellTicket elem = { };

			csv.get(elem.ticket_item_id,           "territory_certificate");
			csv.get(elem.production_rate_modifier, "output_multiple");
			csv.get(elem.capacity_modifier,        "resource_multiple");
			csv.get(elem.soldiers_max,             "territory_hp");
			csv.get(elem.self_healing_rate,        "recovery_hp");

			if(!ticket_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapCellTicket: ticket_item_id = ", elem.ticket_item_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapCellTicket"));
			}
		}
		g_ticket_map = ticket_map;
		handles.push(ticket_map);
		auto servlet = DataSession::create_servlet(TICKET_FILE, Data::encode_csv_as_json(csv, "territory_certificate"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(TERRAIN_FILE);
		const auto terrain_map = boost::make_shared<TerrainMap>();
		while(csv.fetch_row()){
			Data::MapTerrain elem = { };

			csv.get(elem.terrain_id,           "territory_id");

			csv.get(elem.best_resource_id,     "production");
			csv.get(elem.best_production_rate, "output_perminute");
			csv.get(elem.best_capacity,        "resource_max");
			csv.get(elem.buildable,            "construction");
			csv.get(elem.passable,             "mobile");

			if(!terrain_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapTerrain: terrain_id = ", elem.terrain_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapTerrain"));
			}
		}
		g_terrain_map = terrain_map;
		handles.push(terrain_map);
		servlet = DataSession::create_servlet(TERRAIN_FILE, Data::encode_csv_as_json(csv, "territory_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(OVERLAY_FILE);
		const auto overlay_map = boost::make_shared<OverlayMap>();
		while(csv.fetch_row()){
			Data::MapOverlay elem = { };

			csv.get(elem.overlay_id,             "remove_id");

			csv.get(elem.reward_resource_id,     "product");
			csv.get(elem.reward_resource_amount, "number");

			if(!overlay_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapOverlay: overlay_id = ", elem.overlay_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapOverlay"));
			}
		}
		g_overlay_map = overlay_map;
		handles.push(overlay_map);
		servlet = DataSession::create_servlet(OVERLAY_FILE, Data::encode_csv_as_json(csv, "remove_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(START_POINT_FILE);
		const auto start_point_map = boost::make_shared<StartPointMap>();
		while(csv.fetch_row()){
			Data::MapStartPoint elem = { };

			csv.get(elem.start_point_id,   "birth_point_id");

			csv.get(elem.map_coord.first,  "x");
			csv.get(elem.map_coord.second, "y");

			if(!start_point_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapStartPoint: start_point_id = ", elem.start_point_id,
					", map_x = ", elem.map_coord.first, ", map_y = ", elem.map_coord.second);
				DEBUG_THROW(Exception, sslit("Duplicate MapStartPoint"));
			}
		}
		g_start_point_map = start_point_map;
		handles.push(start_point_map);
		// servlet = DataSession::create_servlet(START_POINT_FILE, Data::encode_csv_as_json(csv, "birth_point_id"));
		// handles.push(std::move(servlet));

		csv = Data::sync_load_data(CRATE_FILE);
		const auto crate_map = boost::make_shared<CrateMap>();
		while(csv.fetch_row()){
			Data::MapCrate elem = { };

			csv.get(elem.crate_id, "chest_id");

			Poseidon::JsonArray array;
			csv.get(array, "resource_id");
			elem.resource_amount_key.first  = ResourceId(array.at(0).get<double>());
			elem.resource_amount_key.second = array.at(1).get<double>();

			if(!crate_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapCrate: crate_id = ", elem.crate_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapCrate"));
			}
		}
		g_crate_map = crate_map;
		handles.push(crate_map);
		servlet = DataSession::create_servlet(CRATE_FILE, Data::encode_csv_as_json(csv, "chest_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(CASTLE_FILE);
		const auto castle_map = boost::make_shared<CastleMap>();
		while(csv.fetch_row()){
			Data::MapDefenseBuildingCastle elem = { };

			csv.get(elem.building_level, "building_level");
			read_defense_building_abstract(elem, csv);

			//

			if(!castle_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObjectCastle: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectCastle"));
			}
		}
		g_castle_map = castle_map;
		handles.push(castle_map);
		servlet = DataSession::create_servlet(CASTLE_FILE, Data::encode_csv_as_json(csv, "building_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(DEFENSE_TOWER_FILE);
		const auto defense_tower_map = boost::make_shared<DefenseTowerMap>();
		while(csv.fetch_row()){
			Data::MapDefenseBuildingDefenseTower elem = { };

			csv.get(elem.building_level, "building_level");
			read_defense_building_abstract(elem, csv);

			//

			if(!defense_tower_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObjectDefenseTower: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectDefenseTower"));
			}
		}
		g_defense_tower_map = defense_tower_map;
		handles.push(defense_tower_map);
		servlet = DataSession::create_servlet(DEFENSE_TOWER_FILE, Data::encode_csv_as_json(csv, "building_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(BATTLE_BUNKER_FILE);
		const auto battle_bunker_map = boost::make_shared<BattleBunkerMap>();
		while(csv.fetch_row()){
			Data::MapDefenseBuildingBattleBunker elem = { };

			csv.get(elem.building_level, "building_level");
			read_defense_building_abstract(elem, csv);

			//

			if(!battle_bunker_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObjectBattleBunker: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectBattleBunker"));
			}
		}
		g_battle_bunker_map = battle_bunker_map;
		handles.push(battle_bunker_map);
		servlet = DataSession::create_servlet(BATTLE_BUNKER_FILE, Data::encode_csv_as_json(csv, "building_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(DEFENSE_COMBAT_FILE);
		const auto defense_combat_map = boost::make_shared<DefenseCombatMap>();
		while(csv.fetch_row()){
			Data::MapDefenseCombat elem = { };

			csv.get(elem.defense_combat_id, "id");
			csv.get(elem.soldiers_max,      "force_mnax");
			csv.get(elem.self_healing_rate, "building_recover");

			if(!defense_combat_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapDefenseCombat: defense_combat_id = ", elem.defense_combat_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapDefenseCombat"));
			}
		}
		g_defense_combat_map = defense_combat_map;
		handles.push(defense_combat_map);
		servlet = DataSession::create_servlet(DEFENSE_COMBAT_FILE, Data::encode_csv_as_json(csv, "id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const MapCellBasic> MapCellBasic::get(unsigned map_x, unsigned map_y){
		PROFILE_ME;

		const auto basic_map = g_basic_map.lock();
		if(!basic_map){
			LOG_EMPERY_CENTER_WARNING("MapCellBasicMap has not been loaded.");
			return { };
		}

		const auto it = basic_map->find<0>(std::make_pair(map_x, map_y));
		if(it == basic_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapCellBasic not found: map_x = ", map_x, ", map_y = ", map_y);
			return { };
		}
		return boost::shared_ptr<const MapCellBasic>(basic_map, &*it);
	}
	boost::shared_ptr<const MapCellBasic> MapCellBasic::require(unsigned map_x, unsigned map_y){
		PROFILE_ME;

		auto ret = get(map_x, map_y);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapCellBasic not found: map_x = ", map_x, ", map_y = ", map_y);
			DEBUG_THROW(Exception, sslit("MapCellBasic not found"));
		}
		return ret;
	}

	void MapCellBasic::get_by_overlay_group(std::vector<boost::shared_ptr<const MapCellBasic>> &ret, const std::string &overlay_group_name){
		PROFILE_ME;

		const auto basic_map = g_basic_map.lock();
		if(!basic_map){
			LOG_EMPERY_CENTER_WARNING("MapCellBasicMap has not been loaded.");
			return;
		}

		const auto range = basic_map->equal_range<1>(overlay_group_name);
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			ret.emplace_back(basic_map, &*it);
		}
	}

	boost::shared_ptr<const MapCellTicket> MapCellTicket::get(ItemId ticket_item_id){
		PROFILE_ME;

		const auto ticket_map = g_ticket_map.lock();
		if(!ticket_map){
			LOG_EMPERY_CENTER_WARNING("MapCellTicketMap has not been loaded.");
			return { };
		}

		const auto it = ticket_map->find<0>(ticket_item_id);
		if(it == ticket_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapCellTicket not found: ticket_item_id = ", ticket_item_id);
			return { };
		}
		return boost::shared_ptr<const MapCellTicket>(ticket_map, &*it);
	}
	boost::shared_ptr<const MapCellTicket> MapCellTicket::require(ItemId ticket_item_id){
		PROFILE_ME;

		auto ret = get(ticket_item_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapCellTicket not found: ticket_item_id = ", ticket_item_id);
			DEBUG_THROW(Exception, sslit("MapCellTicket not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapTerrain> MapTerrain::get(TerrainId terrain_id){
		PROFILE_ME;

		const auto terrain_map = g_terrain_map.lock();
		if(!terrain_map){
			LOG_EMPERY_CENTER_WARNING("MapTerrainMap has not been loaded.");
			return { };
		}

		const auto it = terrain_map->find<0>(terrain_id);
		if(it == terrain_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapTerrain not found: terrain_id = ", terrain_id);
			return { };
		}
		return boost::shared_ptr<const MapTerrain>(terrain_map, &*it);
	}
	boost::shared_ptr<const MapTerrain> MapTerrain::require(TerrainId terrain_id){
		PROFILE_ME;

		auto ret = get(terrain_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapTerrain not found: terrain_id = ", terrain_id);
			DEBUG_THROW(Exception, sslit("MapTerrain not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapOverlay> MapOverlay::get(OverlayId overlay_id){
		PROFILE_ME;

		const auto overlay_map = g_overlay_map.lock();
		if(!overlay_map){
			LOG_EMPERY_CENTER_WARNING("MapOverlayMap has not been loaded.");
			return { };
		}

		const auto it = overlay_map->find<0>(overlay_id);
		if(it == overlay_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapOverlay not found: overlay_id = ", overlay_id);
			return { };
		}
		return boost::shared_ptr<const MapOverlay>(overlay_map, &*it);
	}
	boost::shared_ptr<const MapOverlay> MapOverlay::require(OverlayId overlay_id){
		PROFILE_ME;

		auto ret = get(overlay_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapOverlay not found: overlay_id = ", overlay_id);
			DEBUG_THROW(Exception, sslit("MapOverlay not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapStartPoint> MapStartPoint::get(StartPointId start_point_id){
		PROFILE_ME;

		const auto start_point_map = g_start_point_map.lock();
		if(!start_point_map){
			LOG_EMPERY_CENTER_WARNING("MapStartPointMap has not been loaded.");
			return { };
		}

		const auto it = start_point_map->find<0>(start_point_id);
		if(it == start_point_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapStartPoint not found: start_point_id = ", start_point_id);
			return { };
		}
		return boost::shared_ptr<const MapStartPoint>(start_point_map, &*it);
	}
	boost::shared_ptr<const MapStartPoint> MapStartPoint::require(StartPointId start_point_id){
		PROFILE_ME;

		auto ret = get(start_point_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapStartPoint not found: start_point_id = ", start_point_id);
			DEBUG_THROW(Exception, sslit("MapStartPoint not found"));
		}
		return ret;
	}

	void MapStartPoint::get_all(std::vector<boost::shared_ptr<const MapStartPoint>> &ret){
		PROFILE_ME;

		const auto start_point_map = g_start_point_map.lock();
		if(!start_point_map){
			LOG_EMPERY_CENTER_WARNING("MapStartPointMap has not been loaded.");
			return;
		}

		ret.reserve(ret.size() + start_point_map->size());
		for(auto it = start_point_map->begin<1>(); it != start_point_map->end<1>(); ++it){
			ret.emplace_back(start_point_map, &*it);
		}
	}

	boost::shared_ptr<const MapCrate> MapCrate::get(CrateId crate_id){
		PROFILE_ME;

		const auto crate_map = g_crate_map.lock();
		if(!crate_map){
			LOG_EMPERY_CENTER_WARNING("MapCrateMap has not been loaded.");
			return { };
		}

		const auto it = crate_map->find<0>(crate_id);
		if(it == crate_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapCrate not found: crate_id = ", crate_id);
			return { };
		}
		return boost::shared_ptr<const MapCrate>(crate_map, &*it);
	}
	boost::shared_ptr<const MapCrate> MapCrate::require(CrateId crate_id){
		PROFILE_ME;

		auto ret = get(crate_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapCrate not found: crate_id = ", crate_id);
			DEBUG_THROW(Exception, sslit("MapCrate not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapCrate> MapCrate::get_by_resource_amount(ResourceId resource_id, std::uint64_t amount){
		PROFILE_ME;

		const auto crate_map = g_crate_map.lock();
		if(!crate_map){
			LOG_EMPERY_CENTER_WARNING("MapCrateMap has not been loaded.");
			return { };
		}

		auto it = crate_map->upper_bound<1>(std::make_pair(resource_id, amount));
		if(it != crate_map->begin<1>()){
			auto prev = std::prev(it);
			if(prev->resource_amount_key.first == resource_id){
				it = prev;
			}
		}
		if((it == crate_map->end<1>()) || (it->resource_amount_key.first != resource_id)){
			LOG_EMPERY_CENTER_TRACE("MapCrate not found: resource_id = ", resource_id, ", amount = ", amount);
			return { };
		}
		return boost::shared_ptr<const MapCrate>(crate_map, &*it);
	}

	boost::shared_ptr<const MapDefenseBuildingAbstract> MapDefenseBuildingAbstract::get(
		MapObjectTypeId map_object_type_id, unsigned building_level)
	{
		PROFILE_ME;

		switch(map_object_type_id.get()){
		case MapObjectTypeIds::ID_CASTLE.get():
			return MapDefenseBuildingCastle::get(building_level);
		case MapObjectTypeIds::ID_DEFENSE_TOWER.get():
			return MapDefenseBuildingDefenseTower::get(building_level);
		case MapObjectTypeIds::ID_BATTLE_BUNKER.get():
			return MapDefenseBuildingBattleBunker::get(building_level);
		default:
			LOG_EMPERY_CENTER_TRACE("Unhandled defense building id: map_object_type_id = ", map_object_type_id);
			return { };
		}
	}
	boost::shared_ptr<const MapDefenseBuildingAbstract> MapDefenseBuildingAbstract::require(
		MapObjectTypeId map_object_type_id, unsigned building_level)
	{
		PROFILE_ME;

		auto ret = get(map_object_type_id, building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapDefenseBuildingAbstract not found: map_object_type_id = ", map_object_type_id,
				", building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("MapDefenseBuildingAbstract not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapDefenseBuildingCastle> MapDefenseBuildingCastle::get(unsigned building_level){
		PROFILE_ME;

		const auto castle_map = g_castle_map.lock();
		if(!castle_map){
			LOG_EMPERY_CENTER_WARNING("MapDefenseBuildingCastle has not been loaded.");
			return { };
		}

		const auto it = castle_map->find(building_level);
		if(it == castle_map->end()){
			LOG_EMPERY_CENTER_TRACE("MapDefenseBuildingCastle not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const MapDefenseBuildingCastle>(castle_map, &(it->second));
	}
	boost::shared_ptr<const MapDefenseBuildingCastle> MapDefenseBuildingCastle::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapDefenseBuildingCastle not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("MapDefenseBuildingCastle not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapDefenseBuildingDefenseTower> MapDefenseBuildingDefenseTower::get(unsigned building_level){
		PROFILE_ME;

		const auto defense_tower_map = g_defense_tower_map.lock();
		if(!defense_tower_map){
			LOG_EMPERY_CENTER_WARNING("MapDefenseBuildingDefenseTower has not been loaded.");
			return { };
		}

		const auto it = defense_tower_map->find(building_level);
		if(it == defense_tower_map->end()){
			LOG_EMPERY_CENTER_TRACE("MapDefenseBuildingDefenseTower not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const MapDefenseBuildingDefenseTower>(defense_tower_map, &(it->second));
	}
	boost::shared_ptr<const MapDefenseBuildingDefenseTower> MapDefenseBuildingDefenseTower::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapDefenseBuildingDefenseTower not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("MapDefenseBuildingDefenseTower not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapDefenseBuildingBattleBunker> MapDefenseBuildingBattleBunker::get(unsigned building_level){
		PROFILE_ME;

		const auto battle_bunker_map = g_battle_bunker_map.lock();
		if(!battle_bunker_map){
			LOG_EMPERY_CENTER_WARNING("MapDefenseBuildingBattleBunker has not been loaded.");
			return { };
		}

		const auto it = battle_bunker_map->find(building_level);
		if(it == battle_bunker_map->end()){
			LOG_EMPERY_CENTER_TRACE("MapDefenseBuildingBattleBunker not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const MapDefenseBuildingBattleBunker>(battle_bunker_map, &(it->second));
	}
	boost::shared_ptr<const MapDefenseBuildingBattleBunker> MapDefenseBuildingBattleBunker::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapDefenseBuildingBattleBunker not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("MapDefenseBuildingBattleBunker not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapDefenseCombat> MapDefenseCombat::get(DefenseCombatId defense_combat_id){
		PROFILE_ME;

		const auto defense_combat_map = g_defense_combat_map.lock();
		if(!defense_combat_map){
			LOG_EMPERY_CENTER_WARNING("MapDefenseCombat has not been loaded.");
			return { };
		}

		const auto it = defense_combat_map->find<0>(defense_combat_id);
		if(it == defense_combat_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapDefenseCombat not found: defense_combat_id = ", defense_combat_id);
			return { };
		}
		return boost::shared_ptr<const MapDefenseCombat>(defense_combat_map, &*it);
	}
	boost::shared_ptr<const MapDefenseCombat> MapDefenseCombat::require(DefenseCombatId defense_combat_id){
		PROFILE_ME;

		auto ret = get(defense_combat_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapDefenseCombat not found: defense_combat_id = ", defense_combat_id);
			DEBUG_THROW(Exception, sslit("MapDefenseCombat not found"));
		}
		return ret;
	}
}

}
