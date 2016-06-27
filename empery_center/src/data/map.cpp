#include "../precompiled.hpp"
#include "map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "../map_object_type_ids.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(BasicContainer, Data::MapCellBasic,
		UNIQUE_MEMBER_INDEX(map_coord)
	)
	boost::weak_ptr<const BasicContainer> g_basic_container;
	const char BASIC_FILE[] = "map";

	MULTI_INDEX_MAP(TicketContainer, Data::MapCellTicket,
		UNIQUE_MEMBER_INDEX(ticket_item_id)
	)
	boost::weak_ptr<const TicketContainer> g_ticket_container;
	const char TICKET_FILE[] = "Territory_levelup";

	MULTI_INDEX_MAP(TerrainContainer, Data::MapTerrain,
		UNIQUE_MEMBER_INDEX(terrain_id)
	)
	boost::weak_ptr<const TerrainContainer> g_terrain_container;
	const char TERRAIN_FILE[] = "Territory_product";

	MULTI_INDEX_MAP(StartPointContainer, Data::MapStartPoint,
		UNIQUE_MEMBER_INDEX(start_point_id)
		UNIQUE_MEMBER_INDEX(map_coord)
	)
	boost::weak_ptr<const StartPointContainer> g_start_point_container;
	const char START_POINT_FILE[] = "birth_point";

	MULTI_INDEX_MAP(CrateContainer, Data::MapCrate,
		UNIQUE_MEMBER_INDEX(crate_id)
		UNIQUE_MEMBER_INDEX(resource_amount_key)
	)
	boost::weak_ptr<const CrateContainer> g_crate_container;
	const char CRATE_FILE[] = "chest";

	using CastleContainer = boost::container::flat_map<unsigned, Data::MapDefenseBuildingCastle>;
	boost::weak_ptr<const CastleContainer> g_castle_container;
	const char CASTLE_FILE[] = "Building_castel";

	using DefenseTowerContainer = boost::container::flat_map<unsigned, Data::MapDefenseBuildingDefenseTower>;
	boost::weak_ptr<const DefenseTowerContainer> g_defense_tower_container;
	const char DEFENSE_TOWER_FILE[] = "Building_towers";

	using BattleBunkerContainer = boost::container::flat_map<unsigned, Data::MapDefenseBuildingBattleBunker>;
	boost::weak_ptr<const BattleBunkerContainer> g_battle_bunker_container;
	const char BATTLE_BUNKER_FILE[] = "Building_bunker";

	MULTI_INDEX_MAP(DefenseCombatContainer, Data::MapDefenseCombat,
		UNIQUE_MEMBER_INDEX(defense_combat_id)
	)
	boost::weak_ptr<const DefenseCombatContainer> g_defense_combat_container;
	const char DEFENSE_COMBAT_FILE[] = "Building_combat_attributes";

	template<typename ElementT>
	void read_defense_building_abstract(ElementT &elem, const Poseidon::CsvParser &csv){
		csv.get(elem.upgrade_duration, "building_upgrade_time");

		Poseidon::JsonObject object;
		csv.get(object, "building_upgrade_cost");
		elem.upgrade_cost.reserve(object.size());
		for(auto it = object.begin(); it != object.end(); ++it){
			const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
			const auto resource_amount = static_cast<std::uint64_t>(it->second.get<double>());
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
			const auto resource_amount = static_cast<std::uint64_t>(it->second.get<double>());
			if(!elem.debris.emplace(resource_id, resource_amount).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade resource cost: resource_id = ", resource_id);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade resource cost"));
			}
		}

		csv.get(elem.destruct_duration, "building_dismantling_time");
		csv.get(elem.defense_combat_id, "building_combat_attributes");
	}

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(BASIC_FILE);
		const auto basic_container = boost::make_shared<BasicContainer>();
		while(csv.fetch_row()){
			Data::MapCellBasic elem = { };

			Poseidon::JsonArray array;
			csv.get(array, "xy");
			const unsigned x = array.at(0).get<double>();
			const unsigned y = array.at(1).get<double>();
			elem.map_coord = std::make_pair(x, y);

			csv.get(elem.terrain_id,         "property_id");

			if(!basic_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate ContainerCellBasic: x = ", elem.map_coord.first, ", y = ", elem.map_coord.second);
				DEBUG_THROW(Exception, sslit("Duplicate ContainerCellBasic"));
			}
		}
		g_basic_container = basic_container;
		handles.push(basic_container);
		// servlet = DataSession::create_servlet(BASIC_FILE, Data::encode_csv_as_json(csv, "xy"));
		// handles.push(std::move(servlet));

		csv = Data::sync_load_data(TICKET_FILE);
		const auto ticket_container = boost::make_shared<TicketContainer>();
		while(csv.fetch_row()){
			Data::MapCellTicket elem = { };

			csv.get(elem.ticket_item_id,           "territory_certificate");
			csv.get(elem.production_rate_modifier, "output_multiple");
			csv.get(elem.capacity_modifier,        "resource_multiple");
			csv.get(elem.soldiers_max,             "territory_hp");
			csv.get(elem.self_healing_rate,        "recovery_hp");
			csv.get(elem.protectable,              "protect");

			if(!ticket_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate ContainerCellTicket: ticket_item_id = ", elem.ticket_item_id);
				DEBUG_THROW(Exception, sslit("Duplicate ContainerCellTicket"));
			}
		}
		g_ticket_container = ticket_container;
		handles.push(ticket_container);
		auto servlet = DataSession::create_servlet(TICKET_FILE, Data::encode_csv_as_json(csv, "territory_certificate"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(TERRAIN_FILE);
		const auto terrain_container = boost::make_shared<TerrainContainer>();
		while(csv.fetch_row()){
			Data::MapTerrain elem = { };

			csv.get(elem.terrain_id,           "territory_id");

			csv.get(elem.best_resource_id,     "production");
			csv.get(elem.best_production_rate, "output_perminute");
			csv.get(elem.best_capacity,        "resource_max");
			csv.get(elem.buildable,            "construction");
			csv.get(elem.passable,             "mobile");
			csv.get(elem.protection_cost,      "need_fountain");

			if(!terrain_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate ContainerTerrain: terrain_id = ", elem.terrain_id);
				DEBUG_THROW(Exception, sslit("Duplicate ContainerTerrain"));
			}
		}
		g_terrain_container = terrain_container;
		handles.push(terrain_container);
		servlet = DataSession::create_servlet(TERRAIN_FILE, Data::encode_csv_as_json(csv, "territory_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(START_POINT_FILE);
		const auto start_point_container = boost::make_shared<StartPointContainer>();
		while(csv.fetch_row()){
			Data::MapStartPoint elem = { };

			csv.get(elem.start_point_id,   "birth_point_id");

			csv.get(elem.map_coord.first,  "x");
			csv.get(elem.map_coord.second, "y");

			if(!start_point_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate ContainerStartPoint: start_point_id = ", elem.start_point_id,
					", map_x = ", elem.map_coord.first, ", map_y = ", elem.map_coord.second);
				DEBUG_THROW(Exception, sslit("Duplicate ContainerStartPoint"));
			}
		}
		g_start_point_container = start_point_container;
		handles.push(start_point_container);
		// servlet = DataSession::create_servlet(START_POINT_FILE, Data::encode_csv_as_json(csv, "birth_point_id"));
		// handles.push(std::move(servlet));

		csv = Data::sync_load_data(CRATE_FILE);
		const auto crate_container = boost::make_shared<CrateContainer>();
		while(csv.fetch_row()){
			Data::MapCrate elem = { };

			csv.get(elem.crate_id, "chest_id");

			Poseidon::JsonArray array;
			csv.get(array, "resource_id");
			elem.resource_amount_key.first  = ResourceId(array.at(0).get<double>());
			elem.resource_amount_key.second = static_cast<std::uint64_t>(array.at(1).get<double>());

			if(!crate_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate ContainerCrate: crate_id = ", elem.crate_id);
				DEBUG_THROW(Exception, sslit("Duplicate ContainerCrate"));
			}
		}
		g_crate_container = crate_container;
		handles.push(crate_container);
		servlet = DataSession::create_servlet(CRATE_FILE, Data::encode_csv_as_json(csv, "chest_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(CASTLE_FILE);
		const auto castle_container = boost::make_shared<CastleContainer>();
		while(csv.fetch_row()){
			Data::MapDefenseBuildingCastle elem = { };

			csv.get(elem.building_level, "building_level");
			read_defense_building_abstract(elem, csv);

			//

			if(!castle_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate ContainerObjectCastle: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate ContainerObjectCastle"));
			}
		}
		g_castle_container = castle_container;
		handles.push(castle_container);
		servlet = DataSession::create_servlet(CASTLE_FILE, Data::encode_csv_as_json(csv, "building_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(DEFENSE_TOWER_FILE);
		const auto defense_tower_container = boost::make_shared<DefenseTowerContainer>();
		while(csv.fetch_row()){
			Data::MapDefenseBuildingDefenseTower elem = { };

			csv.get(elem.building_level, "building_level");
			read_defense_building_abstract(elem, csv);

			//

			if(!defense_tower_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate ContainerObjectDefenseTower: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate ContainerObjectDefenseTower"));
			}
		}
		g_defense_tower_container = defense_tower_container;
		handles.push(defense_tower_container);
		servlet = DataSession::create_servlet(DEFENSE_TOWER_FILE, Data::encode_csv_as_json(csv, "building_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(BATTLE_BUNKER_FILE);
		const auto battle_bunker_container = boost::make_shared<BattleBunkerContainer>();
		while(csv.fetch_row()){
			Data::MapDefenseBuildingBattleBunker elem = { };

			csv.get(elem.building_level, "building_level");
			read_defense_building_abstract(elem, csv);

			//

			if(!battle_bunker_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate ContainerObjectBattleBunker: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate ContainerObjectBattleBunker"));
			}
		}
		g_battle_bunker_container = battle_bunker_container;
		handles.push(battle_bunker_container);
		servlet = DataSession::create_servlet(BATTLE_BUNKER_FILE, Data::encode_csv_as_json(csv, "building_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(DEFENSE_COMBAT_FILE);
		const auto defense_combat_container = boost::make_shared<DefenseCombatContainer>();
		while(csv.fetch_row()){
			Data::MapDefenseCombat elem = { };

			csv.get(elem.defense_combat_id,    "id");
			csv.get(elem.map_object_weapon_id, "arm_type");
			csv.get(elem.soldiers_max,         "force_mnax");
			csv.get(elem.self_healing_rate,    "building_recover");
			csv.get(elem.hp_per_soldier,       "hp");

			if(!defense_combat_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate ContainerDefenseCombat: defense_combat_id = ", elem.defense_combat_id);
				DEBUG_THROW(Exception, sslit("Duplicate ContainerDefenseCombat"));
			}
		}
		g_defense_combat_container = defense_combat_container;
		handles.push(defense_combat_container);
		servlet = DataSession::create_servlet(DEFENSE_COMBAT_FILE, Data::encode_csv_as_json(csv, "id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const MapCellBasic> MapCellBasic::get(unsigned map_x, unsigned map_y){
		PROFILE_ME;

		const auto basic_container = g_basic_container.lock();
		if(!basic_container){
			LOG_EMPERY_CENTER_WARNING("MapCellBasicContainer has not been loaded.");
			return { };
		}

		const auto it = basic_container->find<0>(std::make_pair(map_x, map_y));
		if(it == basic_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapCellBasic not found: map_x = ", map_x, ", map_y = ", map_y);
			return { };
		}
		return boost::shared_ptr<const MapCellBasic>(basic_container, &*it);
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

	boost::shared_ptr<const MapCellTicket> MapCellTicket::get(ItemId ticket_item_id){
		PROFILE_ME;

		const auto ticket_container = g_ticket_container.lock();
		if(!ticket_container){
			LOG_EMPERY_CENTER_WARNING("MapCellTicketContainer has not been loaded.");
			return { };
		}

		const auto it = ticket_container->find<0>(ticket_item_id);
		if(it == ticket_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapCellTicket not found: ticket_item_id = ", ticket_item_id);
			return { };
		}
		return boost::shared_ptr<const MapCellTicket>(ticket_container, &*it);
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

		const auto terrain_container = g_terrain_container.lock();
		if(!terrain_container){
			LOG_EMPERY_CENTER_WARNING("MapTerrainContainer has not been loaded.");
			return { };
		}

		const auto it = terrain_container->find<0>(terrain_id);
		if(it == terrain_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapTerrain not found: terrain_id = ", terrain_id);
			return { };
		}
		return boost::shared_ptr<const MapTerrain>(terrain_container, &*it);
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

	boost::shared_ptr<const MapStartPoint> MapStartPoint::get(StartPointId start_point_id){
		PROFILE_ME;

		const auto start_point_container = g_start_point_container.lock();
		if(!start_point_container){
			LOG_EMPERY_CENTER_WARNING("MapStartPointContainer has not been loaded.");
			return { };
		}

		const auto it = start_point_container->find<0>(start_point_id);
		if(it == start_point_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapStartPoint not found: start_point_id = ", start_point_id);
			return { };
		}
		return boost::shared_ptr<const MapStartPoint>(start_point_container, &*it);
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

		const auto start_point_container = g_start_point_container.lock();
		if(!start_point_container){
			LOG_EMPERY_CENTER_WARNING("MapStartPointContainer has not been loaded.");
			return;
		}

		ret.reserve(ret.size() + start_point_container->size());
		for(auto it = start_point_container->begin<1>(); it != start_point_container->end<1>(); ++it){
			ret.emplace_back(start_point_container, &*it);
		}
	}

	boost::shared_ptr<const MapCrate> MapCrate::get(CrateId crate_id){
		PROFILE_ME;

		const auto crate_container = g_crate_container.lock();
		if(!crate_container){
			LOG_EMPERY_CENTER_WARNING("MapCrateContainer has not been loaded.");
			return { };
		}

		const auto it = crate_container->find<0>(crate_id);
		if(it == crate_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapCrate not found: crate_id = ", crate_id);
			return { };
		}
		return boost::shared_ptr<const MapCrate>(crate_container, &*it);
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

		const auto crate_container = g_crate_container.lock();
		if(!crate_container){
			LOG_EMPERY_CENTER_WARNING("MapCrateContainer has not been loaded.");
			return { };
		}

		auto it = crate_container->upper_bound<1>(std::make_pair(resource_id, amount));
		if(it != crate_container->begin<1>()){
			auto prev = std::prev(it);
			if(prev->resource_amount_key.first == resource_id){
				it = prev;
			}
		}
		if((it == crate_container->end<1>()) || (it->resource_amount_key.first != resource_id)){
			LOG_EMPERY_CENTER_TRACE("MapCrate not found: resource_id = ", resource_id, ", amount = ", amount);
			return { };
		}
		return boost::shared_ptr<const MapCrate>(crate_container, &*it);
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

		const auto castle_container = g_castle_container.lock();
		if(!castle_container){
			LOG_EMPERY_CENTER_WARNING("MapDefenseBuildingCastleContainer has not been loaded.");
			return { };
		}

		const auto it = castle_container->find(building_level);
		if(it == castle_container->end()){
			LOG_EMPERY_CENTER_TRACE("MapDefenseBuildingCastle not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const MapDefenseBuildingCastle>(castle_container, &(it->second));
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

		const auto defense_tower_container = g_defense_tower_container.lock();
		if(!defense_tower_container){
			LOG_EMPERY_CENTER_WARNING("MapDefenseBuildingDefenseTowerContainer has not been loaded.");
			return { };
		}

		const auto it = defense_tower_container->find(building_level);
		if(it == defense_tower_container->end()){
			LOG_EMPERY_CENTER_TRACE("MapDefenseBuildingDefenseTower not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const MapDefenseBuildingDefenseTower>(defense_tower_container, &(it->second));
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

		const auto battle_bunker_container = g_battle_bunker_container.lock();
		if(!battle_bunker_container){
			LOG_EMPERY_CENTER_WARNING("MapDefenseBuildingBattleBunkerContainer has not been loaded.");
			return { };
		}

		const auto it = battle_bunker_container->find(building_level);
		if(it == battle_bunker_container->end()){
			LOG_EMPERY_CENTER_TRACE("MapDefenseBuildingBattleBunker not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const MapDefenseBuildingBattleBunker>(battle_bunker_container, &(it->second));
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

		const auto defense_combat_container = g_defense_combat_container.lock();
		if(!defense_combat_container){
			LOG_EMPERY_CENTER_WARNING("MapDefenseCombatContainer has not been loaded.");
			return { };
		}

		const auto it = defense_combat_container->find<0>(defense_combat_id);
		if(it == defense_combat_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapDefenseCombat not found: defense_combat_id = ", defense_combat_id);
			return { };
		}
		return boost::shared_ptr<const MapDefenseCombat>(defense_combat_container, &*it);
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
