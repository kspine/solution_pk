#include "../precompiled.hpp"
#include "map_defense.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "../map_object_type_ids.hpp"

namespace EmperyCenter {

namespace {
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
		auto csv = Data::sync_load_data(CASTLE_FILE);
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
		auto servlet = DataSession::create_servlet(CASTLE_FILE, Data::encode_csv_as_json(csv, "building_level"));
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
