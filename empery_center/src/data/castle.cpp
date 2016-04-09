#include "../precompiled.hpp"
#include "castle.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "../building_type_ids.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(CastleBuildingBaseMap, Data::CastleBuildingBase,
		UNIQUE_MEMBER_INDEX(building_base_id)
		MULTI_MEMBER_INDEX(init_level)
	)
	boost::weak_ptr<const CastleBuildingBaseMap> g_building_base_map;
	const char BUILDING_BASE_FILE[] = "castle_block";

	MULTI_INDEX_MAP(CastleBuildingMap, Data::CastleBuilding,
		UNIQUE_MEMBER_INDEX(building_id)
	)
	boost::weak_ptr<const CastleBuildingMap> g_building_map;
	const char BUILDING_FILE[] = "castle_buildingxy";

	using CastleUpgradePrimaryMap = boost::container::flat_map<unsigned, Data::CastleUpgradePrimary>;
	boost::weak_ptr<const CastleUpgradePrimaryMap> g_upgrade_primary_map;
	const char UPGRADE_PRIMARY_FILE[] = "City_Castel";

	using CastleUpgradeStablesMap = boost::container::flat_map<unsigned, Data::CastleUpgradeStables>;
	boost::weak_ptr<const CastleUpgradeStablesMap> g_upgrade_stables_map;
	const char UPGRADE_STABLES_FILE[] = "City_Camp_horse";

	using CastleUpgradeBarracksMap = boost::container::flat_map<unsigned, Data::CastleUpgradeBarracks>;
	boost::weak_ptr<const CastleUpgradeBarracksMap> g_upgrade_barracks_map;
	const char UPGRADE_BARRACKS_FILE[] = "City_Camp_Barracks";

	using CastleUpgradeArcherBarracksMap = boost::container::flat_map<unsigned, Data::CastleUpgradeArcherBarracks>;
	boost::weak_ptr<const CastleUpgradeArcherBarracksMap> g_upgrade_archer_barracks_map;
	const char UPGRADE_ARCHER_BARRACKS_FILE[] = "City_Camp_target";

	using CastleUpgradeWeaponryMap = boost::container::flat_map<unsigned, Data::CastleUpgradeWeaponry>;
	boost::weak_ptr<const CastleUpgradeWeaponryMap> g_upgrade_weaponry_map;
	const char UPGRADE_WEAPONRY_FILE[] = "City_Camp_instrument";

	using CastleUpgradeAcademyMap = boost::container::flat_map<unsigned, Data::CastleUpgradeAcademy>;
	boost::weak_ptr<const CastleUpgradeAcademyMap> g_upgrade_academy_map;
	const char UPGRADE_ACADEMY_FILE[] = "City_College";

	using CastleUpgradeCivilianMap = boost::container::flat_map<unsigned, Data::CastleUpgradeCivilian>;
	boost::weak_ptr<const CastleUpgradeCivilianMap> g_upgrade_civilian_map;
	const char UPGRADE_CIVILIAN_FILE[] = "City_House";

	using CastleUpgradeWarehouseMap = boost::container::flat_map<unsigned, Data::CastleUpgradeWarehouse>;
	boost::weak_ptr<const CastleUpgradeWarehouseMap> g_upgrade_warehouse_map;
	const char UPGRADE_WAREHOUSE_FILE[] = "City_Storage";

	using CastleUpgradeCitadelWallMap = boost::container::flat_map<unsigned, Data::CastleUpgradeCitadelWall>;
	boost::weak_ptr<const CastleUpgradeCitadelWallMap> g_upgrade_citadel_wall_map;
	const char UPGRADE_CITADEL_WALL_FILE[] = "City_Wall";

	using CastleUpgradeDefenseTowerMap = boost::container::flat_map<unsigned, Data::CastleUpgradeDefenseTower>;
	boost::weak_ptr<const CastleUpgradeDefenseTowerMap> g_upgrade_defense_tower_map;
	const char UPGRADE_DEFENSE_TOWER_FILE[] = "City_Tower";

	using CastleUpgradeParadeGroundMap = boost::container::flat_map<unsigned, Data::CastleUpgradeParadeGround>;
	boost::weak_ptr<const CastleUpgradeParadeGroundMap> g_upgrade_parade_ground_map;
	const char UPGRADE_PARADE_GROUND_FILE[] = "City_Flied";

	using CastleUpgradeBootCampMap = boost::container::flat_map<unsigned, Data::CastleUpgradeBootCamp>;
	boost::weak_ptr<const CastleUpgradeBootCampMap> g_upgrade_boot_camp_map;
	const char UPGRADE_BOOT_CAMP_FILE[] = "City_March";

	using CastleUpgradeMedicalTentMap = boost::container::flat_map<unsigned, Data::CastleUpgradeMedicalTent>;
	boost::weak_ptr<const CastleUpgradeMedicalTentMap> g_upgrade_medical_tent_map;
	const char UPGRADE_MEDICAL_TENT_FILE[] = "City_Wounded_arm";

	using CastleUpgradeHarvestWorkshopMap = boost::container::flat_map<unsigned, Data::CastleUpgradeHarvestWorkshop>;
	boost::weak_ptr<const CastleUpgradeHarvestWorkshopMap> g_upgrade_harvest_workshop_map;
	const char UPGRADE_HARVEST_WORKSHOP_FILE[] = "City_Collection";

	using CastleUpgradeWarWorkshopMap = boost::container::flat_map<unsigned, Data::CastleUpgradeWarWorkshop>;
	boost::weak_ptr<const CastleUpgradeWarWorkshopMap> g_upgrade_war_workshop_map;
	const char UPGRADE_WAR_WORKSHOP_FILE[] = "City_Def";

	using CastleUpgradeTreeMap = boost::container::flat_map<unsigned, Data::CastleUpgradeTree>;
	boost::weak_ptr<const CastleUpgradeTreeMap> g_upgrade_tree_map;
	const char UPGRADE_TREE_FILE[] = "City_Tree";

	MULTI_INDEX_MAP(CastleTechMap, Data::CastleTech,
		UNIQUE_MEMBER_INDEX(tech_id_level)
	)
	boost::weak_ptr<const CastleTechMap> g_tech_map;
	const char TECH_FILE[] = "City_College_tech";

	MULTI_INDEX_MAP(CastleResourceMap, Data::CastleResource,
		UNIQUE_MEMBER_INDEX(resource_id)
		MULTI_MEMBER_INDEX(locked_resource_id)
		MULTI_MEMBER_INDEX(carried_attribute_id)
		MULTI_MEMBER_INDEX(init_amount)
		MULTI_MEMBER_INDEX(auto_inc_type)
	)
	boost::weak_ptr<const CastleResourceMap> g_resource_map;
	const char RESOURCE_FILE[] = "initial_material";

	template<typename ElementT>
	void read_upgrade_abstract(ElementT &elem, const Poseidon::CsvParser &csv){
		csv.get(elem.upgrade_duration, "levelup_time");

		Poseidon::JsonObject object;
		csv.get(object, "need_resource");
		elem.upgrade_cost.reserve(object.size());
		for(auto it = object.begin(); it != object.end(); ++it){
			const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
			const auto count = static_cast<std::uint64_t>(it->second.get<double>());
			if(!elem.upgrade_cost.emplace(resource_id, count).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade cost: resource_id = ", resource_id);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade cost"));
			}
		}

		object.clear();
		csv.get(object, "need");
		elem.prerequisite.reserve(object.size());
		for(auto it = object.begin(); it != object.end(); ++it){
			const auto building_id = boost::lexical_cast<BuildingId>(it->first);
			const auto building_level = static_cast<unsigned>(it->second.get<double>());
			if(!elem.prerequisite.emplace(building_id, building_level).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate prerequisite: building_id = ", building_id);
				DEBUG_THROW(Exception, sslit("Duplicate prerequisite"));
			}
		}

		csv.get(elem.prosperity_points, "prosperity");
		csv.get(elem.destruct_duration, "demolition");
	}

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(BUILDING_BASE_FILE);
		const auto building_base_map = boost::make_shared<CastleBuildingBaseMap>();
		while(csv.fetch_row()){
			Data::CastleBuildingBase elem = { };

			csv.get(elem.building_base_id, "index");

			Poseidon::JsonArray array;
			csv.get(array, "building_id");
			elem.buildings_allowed.reserve(array.size());
			for(auto it = array.begin(); it != array.end(); ++it){
				const auto building_id = BuildingId(it->get<double>());
				if(!elem.buildings_allowed.emplace(building_id).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate building allowed: building_id = ", building_id);
					DEBUG_THROW(Exception, sslit("Duplicate building allowed"));
				}
			}

			csv.get(elem.init_level,                "initial_level");
			csv.get(elem.init_building_id_override, "tree");

			Poseidon::JsonObject object;
			csv.get(object, "need");
			elem.operation_prerequisite.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto building_id = boost::lexical_cast<BuildingId>(it->first);
				const auto building_level = static_cast<unsigned>(it->second.get<double>());
				if(!elem.operation_prerequisite.emplace(building_id, building_level).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate operation prerequisite: building_id = ", building_id);
					DEBUG_THROW(Exception, sslit("Duplicate operation prerequisite"));
				}
			}

			if(!building_base_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate building base: building_base_id = ", elem.building_base_id);
				DEBUG_THROW(Exception, sslit("Duplicate building base"));
			}
		}
		g_building_base_map = building_base_map;
		handles.push(building_base_map);
		auto servlet = DataSession::create_servlet(BUILDING_BASE_FILE, Data::encode_csv_as_json(csv, "index"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(BUILDING_FILE);
		const auto building_map = boost::make_shared<CastleBuildingMap>();
		while(csv.fetch_row()){
			Data::CastleBuilding elem = { };

			csv.get(elem.building_id, "id");
			csv.get(elem.build_limit, "num");
			csv.get(elem.type,        "type");

			if(!building_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate building: building_id = ", elem.building_id);
				DEBUG_THROW(Exception, sslit("Duplicate building"));
			}
		}
		g_building_map = building_map;
		handles.push(building_map);
		servlet = DataSession::create_servlet(BUILDING_FILE, Data::encode_csv_as_json(csv, "id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_PRIMARY_FILE);
		const auto upgrade_primary_map = boost::make_shared<CastleUpgradePrimaryMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradePrimary elem = { };

			csv.get(elem.building_level, "castel_level");
			read_upgrade_abstract(elem, csv);

			csv.get(elem.max_map_cell_count,        "territory_number");
			csv.get(elem.max_map_cell_distance,     "range");
			csv.get(elem.max_immigrant_group_count, "people");

			if(!upgrade_primary_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradePrimary: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradePrimary"));
			}
		}
		g_upgrade_primary_map = upgrade_primary_map;
		handles.push(upgrade_primary_map);
		servlet = DataSession::create_servlet(UPGRADE_PRIMARY_FILE, Data::encode_csv_as_json(csv, "castel_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_STABLES_FILE);
		const auto upgrade_stables_map = boost::make_shared<CastleUpgradeStablesMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeStables elem = { };

			csv.get(elem.building_level, "camp_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_stables_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeStables: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeStables"));
			}
		}
		g_upgrade_stables_map = upgrade_stables_map;
		handles.push(upgrade_stables_map);
		servlet = DataSession::create_servlet(UPGRADE_STABLES_FILE, Data::encode_csv_as_json(csv, "camp_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_BARRACKS_FILE);
		const auto upgrade_barracks_map = boost::make_shared<CastleUpgradeBarracksMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeBarracks elem = { };

			csv.get(elem.building_level, "camp_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_barracks_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeBarracks: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeBarracks"));
			}
		}
		g_upgrade_barracks_map = upgrade_barracks_map;
		handles.push(upgrade_barracks_map);
		servlet = DataSession::create_servlet(UPGRADE_BARRACKS_FILE, Data::encode_csv_as_json(csv, "camp_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_ARCHER_BARRACKS_FILE);
		const auto upgrade_archer_barracks_map = boost::make_shared<CastleUpgradeArcherBarracksMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeArcherBarracks elem = { };

			csv.get(elem.building_level, "camp_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_archer_barracks_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeArcherBarracks: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeArcherBarracks"));
			}
		}
		g_upgrade_archer_barracks_map = upgrade_archer_barracks_map;
		handles.push(upgrade_archer_barracks_map);
		servlet = DataSession::create_servlet(UPGRADE_ARCHER_BARRACKS_FILE, Data::encode_csv_as_json(csv, "camp_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_WEAPONRY_FILE);
		const auto upgrade_weaponry_map = boost::make_shared<CastleUpgradeWeaponryMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeWeaponry elem = { };

			csv.get(elem.building_level, "camp_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_weaponry_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeWeaponry: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeWeaponry"));
			}
		}
		g_upgrade_weaponry_map = upgrade_weaponry_map;
		handles.push(upgrade_weaponry_map);
		servlet = DataSession::create_servlet(UPGRADE_WEAPONRY_FILE, Data::encode_csv_as_json(csv, "camp_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_ACADEMY_FILE);
		const auto upgrade_academy_map = boost::make_shared<CastleUpgradeAcademyMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeAcademy elem = { };

			csv.get(elem.building_level, "college_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_academy_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeAcademy: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeAcademy"));
			}
		}
		g_upgrade_academy_map = upgrade_academy_map;
		handles.push(upgrade_academy_map);
		servlet = DataSession::create_servlet(UPGRADE_ACADEMY_FILE, Data::encode_csv_as_json(csv, "college_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_CIVILIAN_FILE);
		const auto upgrade_civilian_map = boost::make_shared<CastleUpgradeCivilianMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeCivilian elem = { };

			csv.get(elem.building_level, "house_level");
			read_upgrade_abstract(elem, csv);

			csv.get(elem.population_production_rate, "population_produce");
			csv.get(elem.population_capacity,        "population_max");

			if(!upgrade_civilian_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeCivilian: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeCivilian"));
			}
		}
		g_upgrade_civilian_map = upgrade_civilian_map;
		handles.push(upgrade_civilian_map);
		servlet = DataSession::create_servlet(UPGRADE_CIVILIAN_FILE, Data::encode_csv_as_json(csv, "house_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_WAREHOUSE_FILE);
		const auto upgrade_warehouse_map = boost::make_shared<CastleUpgradeWarehouseMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeWarehouse elem = { };

			csv.get(elem.building_level, "storage_level");
			read_upgrade_abstract(elem, csv);

			Poseidon::JsonObject object;
			csv.get(object, "resource_max");
			elem.max_resource_amounts.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.max_resource_amounts.emplace(resource_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate resource: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate resource"));
				}
			}

			if(!upgrade_warehouse_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeWarehouse: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeWarehouse"));
			}
		}
		g_upgrade_warehouse_map = upgrade_warehouse_map;
		handles.push(upgrade_warehouse_map);
		servlet = DataSession::create_servlet(UPGRADE_WAREHOUSE_FILE, Data::encode_csv_as_json(csv, "storage_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_CITADEL_WALL_FILE);
		const auto upgrade_citadel_wall_map = boost::make_shared<CastleUpgradeCitadelWallMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeCitadelWall elem = { };

			csv.get(elem.building_level, "wall_level");
			read_upgrade_abstract(elem, csv);

			csv.get(elem.strength, "troops");
			csv.get(elem.armor, "defence");

			if(!upgrade_citadel_wall_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeCitadelWall: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeCitadelWall"));
			}
		}
		g_upgrade_citadel_wall_map = upgrade_citadel_wall_map;
		handles.push(upgrade_citadel_wall_map);
		servlet = DataSession::create_servlet(UPGRADE_CITADEL_WALL_FILE, Data::encode_csv_as_json(csv, "wall_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_DEFENSE_TOWER_FILE);
		const auto upgrade_defense_tower_map = boost::make_shared<CastleUpgradeDefenseTowerMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeDefenseTower elem = { };

			csv.get(elem.building_level, "tower_level");
			read_upgrade_abstract(elem, csv);

			csv.get(elem.firepower, "atk");

			if(!upgrade_defense_tower_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeDefenseTower: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeDefenseTower"));
			}
		}
		g_upgrade_defense_tower_map = upgrade_defense_tower_map;
		handles.push(upgrade_defense_tower_map);
		servlet = DataSession::create_servlet(UPGRADE_DEFENSE_TOWER_FILE, Data::encode_csv_as_json(csv, "tower_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_PARADE_GROUND_FILE);
		const auto upgrade_parade_ground_map = boost::make_shared<CastleUpgradeParadeGroundMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeParadeGround elem = { };

			csv.get(elem.building_level, "filed_level");
			read_upgrade_abstract(elem, csv);

			csv.get(elem.max_battalion_count, "force_limit");

			if(!upgrade_parade_ground_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeParadeGround: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeParadeGround"));
			}
		}
		g_upgrade_parade_ground_map = upgrade_parade_ground_map;
		handles.push(upgrade_parade_ground_map);
		servlet = DataSession::create_servlet(UPGRADE_PARADE_GROUND_FILE, Data::encode_csv_as_json(csv, "filed_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_BOOT_CAMP_FILE);
		const auto upgrade_boot_camp_map = boost::make_shared<CastleUpgradeBootCampMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeBootCamp elem = { };

			csv.get(elem.building_level, "march_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_boot_camp_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeBootCamp: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeBootCamp"));
			}
		}
		g_upgrade_boot_camp_map = upgrade_boot_camp_map;
		handles.push(upgrade_boot_camp_map);
		servlet = DataSession::create_servlet(UPGRADE_BOOT_CAMP_FILE, Data::encode_csv_as_json(csv, "march_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_MEDICAL_TENT_FILE);
		const auto upgrade_medical_tent_map = boost::make_shared<CastleUpgradeMedicalTentMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeMedicalTent elem = { };

			csv.get(elem.building_level, "wounded_arm_level");
			read_upgrade_abstract(elem, csv);

			csv.get(elem.max_wounded_soldier_count, "wounded_max");

			if(!upgrade_medical_tent_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeMedicalTent: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeMedicalTent"));
			}
		}
		g_upgrade_medical_tent_map = upgrade_medical_tent_map;
		handles.push(upgrade_medical_tent_map);
		servlet = DataSession::create_servlet(UPGRADE_MEDICAL_TENT_FILE, Data::encode_csv_as_json(csv, "wounded_arm_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_HARVEST_WORKSHOP_FILE);
		const auto upgrade_harvest_workshop_map = boost::make_shared<CastleUpgradeHarvestWorkshopMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeHarvestWorkshop elem = { };

			csv.get(elem.building_level, "collection_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_harvest_workshop_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeHarvestWorkshop: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeHarvestWorkshop"));
			}
		}
		g_upgrade_harvest_workshop_map = upgrade_harvest_workshop_map;
		handles.push(upgrade_harvest_workshop_map);
		servlet = DataSession::create_servlet(UPGRADE_HARVEST_WORKSHOP_FILE, Data::encode_csv_as_json(csv, "collection_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_WAR_WORKSHOP_FILE);
		const auto upgrade_war_workshop_map = boost::make_shared<CastleUpgradeWarWorkshopMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeWarWorkshop elem = { };

			csv.get(elem.building_level, "city_def_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_war_workshop_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeWarWorkshop: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeWarWorkshop"));
			}
		}
		g_upgrade_war_workshop_map = upgrade_war_workshop_map;
		handles.push(upgrade_war_workshop_map);
		servlet = DataSession::create_servlet(UPGRADE_WAR_WORKSHOP_FILE, Data::encode_csv_as_json(csv, "city_def_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_TREE_FILE);
		const auto upgrade_tree_map = boost::make_shared<CastleUpgradeTreeMap>();
		while(csv.fetch_row()){
			Data::CastleUpgradeTree elem = { };

			csv.get(elem.building_level, "tree_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_tree_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeTree: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeTree"));
			}
		}
		g_upgrade_tree_map = upgrade_tree_map;
		handles.push(upgrade_tree_map);
		servlet = DataSession::create_servlet(UPGRADE_TREE_FILE, Data::encode_csv_as_json(csv, "tree_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(TECH_FILE);
		const auto tech_map = boost::make_shared<CastleTechMap>();
		while(csv.fetch_row()){
			Data::CastleTech elem = { };

			Poseidon::JsonArray array;
			csv.get(array, "tech_id_level");
			elem.tech_id_level.first = TechId(array.at(0).get<double>());
			elem.tech_id_level.second = array.at(1).get<double>();

			csv.get(elem.upgrade_duration, "levelup_time");

			Poseidon::JsonObject object;
			csv.get(object, "need_resource");
			elem.upgrade_cost.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.upgrade_cost.emplace(resource_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate upgrade cost: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate upgrade cost"));
				}
			}

			object.clear();
			csv.get(object, "need");
			elem.prerequisite.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto building_id = boost::lexical_cast<BuildingId>(it->first);
				const auto building_level = static_cast<unsigned>(it->second.get<double>());
				if(!elem.prerequisite.emplace(building_id, building_level).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate prerequisite: building_id = ", building_id);
					DEBUG_THROW(Exception, sslit("Duplicate prerequisite"));
				}
			}

			object.clear();
			csv.get(object, "level_open");
			elem.display_prerequisite.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto building_id = boost::lexical_cast<BuildingId>(it->first);
				const auto building_level = static_cast<unsigned>(it->second.get<double>());
				if(!elem.display_prerequisite.emplace(building_id, building_level).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate display prerequisite: building_id = ", building_id);
					DEBUG_THROW(Exception, sslit("Duplicate display prerequisite"));
				}
			}

			object.clear();
			csv.get(object, "tech_effect");
			elem.attributes.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto attribute_id = boost::lexical_cast<AttributeId>(it->first);
				const auto value = it->second.get<double>();
				if(!elem.attributes.emplace(attribute_id, value).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate attribute: attribute_id = ", attribute_id);
					DEBUG_THROW(Exception, sslit("Duplicate attribute"));
				}
			}

			if(!tech_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate tech: tech_id = ", elem.tech_id_level.first, ", tech_level = ", elem.tech_id_level.second);
				DEBUG_THROW(Exception, sslit("Duplicate tech"));
			}
		}
		g_tech_map = tech_map;
		handles.push(tech_map);
		servlet = DataSession::create_servlet(TECH_FILE, Data::encode_csv_as_json(csv, "tech_id_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(RESOURCE_FILE);
		const auto resource_map = boost::make_shared<CastleResourceMap>();
		while(csv.fetch_row()){
			Data::CastleResource elem = { };

			csv.get(elem.resource_id,          "material_id");

			csv.get(elem.init_amount,          "number");
			csv.get(elem.producible,           "producible");

			csv.get(elem.locked_resource_id,   "lock_material_id");
			csv.get(elem.undeployed_item_id,   "item_id");

			csv.get(elem.carried_attribute_id, "weight_id");

			std::string str;
			csv.get(str, "autoinc_type");
			if(::strcasecmp(str.c_str(), "none") == 0){
				elem.auto_inc_type = elem.AIT_NONE;
			} else if(::strcasecmp(str.c_str(), "hourly") == 0){
				elem.auto_inc_type = elem.AIT_HOURLY;
			} else if(::strcasecmp(str.c_str(), "daily") == 0){
				elem.auto_inc_type = elem.AIT_DAILY;
			} else if(::strcasecmp(str.c_str(), "weekly") == 0){
				elem.auto_inc_type = elem.AIT_WEEKLY;
			} else if(::strcasecmp(str.c_str(), "periodic") == 0){
				elem.auto_inc_type = elem.AIT_PERIODIC;
			} else {
				LOG_EMPERY_CENTER_WARNING("Unknown resource auto increment type: ", str);
				DEBUG_THROW(Exception, sslit("Unknown resource auto increment type"));
			}
			csv.get(elem.auto_inc_offset,      "autoinc_time");
			csv.get(elem.auto_inc_step,        "autoinc_step");
			csv.get(elem.auto_inc_bound,       "autoinc_bound");

			if(!resource_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate initial resource: resource_id = ", elem.resource_id,
					", locked_resource_id = ", elem.locked_resource_id);
				DEBUG_THROW(Exception, sslit("Duplicate initial resource"));
			}
		}
		g_resource_map = resource_map;
		handles.push(resource_map);
		servlet = DataSession::create_servlet(RESOURCE_FILE, Data::encode_csv_as_json(csv, "material_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const CastleBuildingBase> CastleBuildingBase::get(BuildingBaseId building_base_id){
		PROFILE_ME;

		const auto building_base_map = g_building_base_map.lock();
		if(!building_base_map){
			LOG_EMPERY_CENTER_WARNING("CastleBuildingBaseMap has not been loaded.");
			return { };
		}

		const auto it = building_base_map->find<0>(building_base_id);
		if(it == building_base_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("CastleBuildingBase not found: building_base_id = ", building_base_id);
			return { };
		}
		return boost::shared_ptr<const CastleBuildingBase>(building_base_map, &*it);
	}
	boost::shared_ptr<const CastleBuildingBase> CastleBuildingBase::require(BuildingBaseId building_base_id){
		PROFILE_ME;

		auto ret = get(building_base_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleBuildingBase not found: building_base_id = ", building_base_id);
			DEBUG_THROW(Exception, sslit("CastleBuildingBase not found"));
		}
		return ret;
	}

	void CastleBuildingBase::get_init(std::vector<boost::shared_ptr<const CastleBuildingBase>> &ret){
		PROFILE_ME;

		const auto building_base_map = g_building_base_map.lock();
		if(!building_base_map){
			LOG_EMPERY_CENTER_WARNING("CastleBuildingBaseMap has not been loaded.");
			return;
		}

		const auto range = std::make_pair(building_base_map->upper_bound<1>(0), building_base_map->end<1>());
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			ret.emplace_back(building_base_map, &*it);
		}
	}

	boost::shared_ptr<const CastleBuilding> CastleBuilding::get(BuildingId building_id){
		PROFILE_ME;

		const auto building_map = g_building_map.lock();
		if(!building_map){
			LOG_EMPERY_CENTER_WARNING("CastleBuildingMap has not been loaded.");
			return { };
		}

		const auto it = building_map->find<0>(building_id);
		if(it == building_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("CastleBuilding not found: building_id = ", building_id);
			return { };
		}
		return boost::shared_ptr<const CastleBuilding>(building_map, &*it);
	}
	boost::shared_ptr<const CastleBuilding> CastleBuilding::require(BuildingId building_id){
		PROFILE_ME;

		auto ret = get(building_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleBuilding not found: building_id = ", building_id);
			DEBUG_THROW(Exception, sslit("CastleBuilding base not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeAbstract> CastleUpgradeAbstract::get(BuildingTypeId type, unsigned building_level){
		PROFILE_ME;

		switch(type.get()){
		case BuildingTypeIds::ID_PRIMARY.get():
			return CastleUpgradePrimary::get(building_level);
		case BuildingTypeIds::ID_ACADEMY.get():
			return CastleUpgradeAcademy::get(building_level);
		case BuildingTypeIds::ID_CIVILIAN.get():
			return CastleUpgradeCivilian::get(building_level);
//		case BuildingTypeIds::ID_WARCHTOWER.get():
//			return CastleUpgradeWatchtower::get(building_level);
		case BuildingTypeIds::ID_WAREHOUSE.get():
			return CastleUpgradeWarehouse::get(building_level);
		case BuildingTypeIds::ID_CITADEL_WALL.get():
			return CastleUpgradeCitadelWall::get(building_level);
		case BuildingTypeIds::ID_DEFENSE_TOWER.get():
			return CastleUpgradeDefenseTower::get(building_level);
		case BuildingTypeIds::ID_PARADE_GROUND.get():
			return CastleUpgradeParadeGround::get(building_level);
		case BuildingTypeIds::ID_STABLES.get():
			return CastleUpgradeStables::get(building_level);
		case BuildingTypeIds::ID_BARRACKS.get():
			return CastleUpgradeBarracks::get(building_level);
		case BuildingTypeIds::ID_ARCHER_BARRACKS.get():
			return CastleUpgradeArcherBarracks::get(building_level);
		case BuildingTypeIds::ID_WEAPONRY.get():
			return CastleUpgradeWeaponry::get(building_level);
		case BuildingTypeIds::ID_BOOT_CAMP.get():
			return CastleUpgradeBootCamp::get(building_level);
		case BuildingTypeIds::ID_MEDICAL_TENT.get():
			return CastleUpgradeMedicalTent::get(building_level);
		case BuildingTypeIds::ID_HARVEST_WORKSHOP.get():
			return CastleUpgradeHarvestWorkshop::get(building_level);
		case BuildingTypeIds::ID_WAR_WORKSHOP.get():
			return CastleUpgradeWarWorkshop::get(building_level);
		case BuildingTypeIds::ID_TREE.get():
			return CastleUpgradeTree::get(building_level);
		default:
			LOG_EMPERY_CENTER_DEBUG("Unhandled building type: type = ", type);
			return { };
		}
	}
	boost::shared_ptr<const CastleUpgradeAbstract> CastleUpgradeAbstract::require(BuildingTypeId type, unsigned building_level){
		PROFILE_ME;

		auto ret = get(type, building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeAbstract not found: type = ", type, ", building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeAbstract not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradePrimary> CastleUpgradePrimary::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_primary_map = g_upgrade_primary_map.lock();
		if(!upgrade_primary_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradePrimaryMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_primary_map->find(building_level);
		if(it == upgrade_primary_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradePrimary not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradePrimary>(upgrade_primary_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradePrimary> CastleUpgradePrimary::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradePrimary not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradePrimary not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeStables> CastleUpgradeStables::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_stables_map = g_upgrade_stables_map.lock();
		if(!upgrade_stables_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeStablesMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_stables_map->find(building_level);
		if(it == upgrade_stables_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeStables not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeStables>(upgrade_stables_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeStables> CastleUpgradeStables::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeStables not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeStables not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeBarracks> CastleUpgradeBarracks::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_barracks_map = g_upgrade_barracks_map.lock();
		if(!upgrade_barracks_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeBarracksMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_barracks_map->find(building_level);
		if(it == upgrade_barracks_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeBarracks not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeBarracks>(upgrade_barracks_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeBarracks> CastleUpgradeBarracks::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeBarracks not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeBarracks not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeArcherBarracks> CastleUpgradeArcherBarracks::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_archer_barracks_map = g_upgrade_archer_barracks_map.lock();
		if(!upgrade_archer_barracks_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeArcherBarracksMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_archer_barracks_map->find(building_level);
		if(it == upgrade_archer_barracks_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeArcherBarracks not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeArcherBarracks>(upgrade_archer_barracks_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeArcherBarracks> CastleUpgradeArcherBarracks::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeArcherBarracks not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeArcherBarracks not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeWeaponry> CastleUpgradeWeaponry::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_weaponry_map = g_upgrade_weaponry_map.lock();
		if(!upgrade_weaponry_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeWeaponryMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_weaponry_map->find(building_level);
		if(it == upgrade_weaponry_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeWeaponry not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeWeaponry>(upgrade_weaponry_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeWeaponry> CastleUpgradeWeaponry::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeWeaponry not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeWeaponry not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeAcademy> CastleUpgradeAcademy::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_academy_map = g_upgrade_academy_map.lock();
		if(!upgrade_academy_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeAcademyMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_academy_map->find(building_level);
		if(it == upgrade_academy_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeAcademy not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeAcademy>(upgrade_academy_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeAcademy> CastleUpgradeAcademy::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeAcademy not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeAcademy not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeCivilian> CastleUpgradeCivilian::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_civilian_map = g_upgrade_civilian_map.lock();
		if(!upgrade_civilian_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeCivilianMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_civilian_map->find(building_level);
		if(it == upgrade_civilian_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeCivilian not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeCivilian>(upgrade_civilian_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeCivilian> CastleUpgradeCivilian::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeCivilian not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeCivilian not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeWarehouse> CastleUpgradeWarehouse::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_warehouse_map = g_upgrade_warehouse_map.lock();
		if(!upgrade_warehouse_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeWarehouseMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_warehouse_map->find(building_level);
		if(it == upgrade_warehouse_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeWarehouse not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeWarehouse>(upgrade_warehouse_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeWarehouse> CastleUpgradeWarehouse::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeWarehouse not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeWarehouse not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeCitadelWall> CastleUpgradeCitadelWall::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_citadel_wall_map = g_upgrade_citadel_wall_map.lock();
		if(!upgrade_citadel_wall_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeCitadelWallMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_citadel_wall_map->find(building_level);
		if(it == upgrade_citadel_wall_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeCitadelWall not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeCitadelWall>(upgrade_citadel_wall_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeCitadelWall> CastleUpgradeCitadelWall::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeCitadelWall not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeCitadelWall not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeDefenseTower> CastleUpgradeDefenseTower::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_defense_tower_map = g_upgrade_defense_tower_map.lock();
		if(!upgrade_defense_tower_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeDefenseTowerMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_defense_tower_map->find(building_level);
		if(it == upgrade_defense_tower_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeDefenseTower not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeDefenseTower>(upgrade_defense_tower_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeDefenseTower> CastleUpgradeDefenseTower::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeDefenseTower not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeDefenseTower not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeParadeGround> CastleUpgradeParadeGround::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_parade_ground_map = g_upgrade_parade_ground_map.lock();
		if(!upgrade_parade_ground_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeParadeGroundMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_parade_ground_map->find(building_level);
		if(it == upgrade_parade_ground_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeParadeGround not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeParadeGround>(upgrade_parade_ground_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeParadeGround> CastleUpgradeParadeGround::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeParadeGround not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeParadeGround not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeBootCamp> CastleUpgradeBootCamp::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_boot_camp_map = g_upgrade_boot_camp_map.lock();
		if(!upgrade_boot_camp_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeBootCampMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_boot_camp_map->find(building_level);
		if(it == upgrade_boot_camp_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeBootCamp not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeBootCamp>(upgrade_boot_camp_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeBootCamp> CastleUpgradeBootCamp::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeBootCamp not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeBootCamp not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeMedicalTent> CastleUpgradeMedicalTent::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_medical_tent_map = g_upgrade_medical_tent_map.lock();
		if(!upgrade_medical_tent_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeMedicalTentMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_medical_tent_map->find(building_level);
		if(it == upgrade_medical_tent_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeMedicalTent not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeMedicalTent>(upgrade_medical_tent_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeMedicalTent> CastleUpgradeMedicalTent::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeMedicalTent not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeMedicalTent not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeHarvestWorkshop> CastleUpgradeHarvestWorkshop::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_harvest_workshop_map = g_upgrade_harvest_workshop_map.lock();
		if(!upgrade_harvest_workshop_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeHarvestWorkshopMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_harvest_workshop_map->find(building_level);
		if(it == upgrade_harvest_workshop_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeHarvestWorkshop not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeHarvestWorkshop>(upgrade_harvest_workshop_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeHarvestWorkshop> CastleUpgradeHarvestWorkshop::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeHarvestWorkshop not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeHarvestWorkshop not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeWarWorkshop> CastleUpgradeWarWorkshop::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_war_workshop_map = g_upgrade_war_workshop_map.lock();
		if(!upgrade_war_workshop_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeWarWorkshopMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_war_workshop_map->find(building_level);
		if(it == upgrade_war_workshop_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeWarWorkshop not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeWarWorkshop>(upgrade_war_workshop_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeWarWorkshop> CastleUpgradeWarWorkshop::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeWarWorkshop not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeWarWorkshop not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeTree> CastleUpgradeTree::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_tree_map = g_upgrade_tree_map.lock();
		if(!upgrade_tree_map){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeTreeMap has not been loaded.");
			return { };
		}

		const auto it = upgrade_tree_map->find(building_level);
		if(it == upgrade_tree_map->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeTree not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeTree>(upgrade_tree_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeTree> CastleUpgradeTree::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeTree not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeTree not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleTech> CastleTech::get(TechId tech_id, unsigned tech_level){
		PROFILE_ME;

		const auto tech_map = g_tech_map.lock();
		if(!tech_map){
			LOG_EMPERY_CENTER_WARNING("CastleTechMap has not been loaded.");
			return { };
		}

		const auto it = tech_map->find<0>(std::make_pair(tech_id, tech_level));
		if(it == tech_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("CastleTech not found: tech_id = ", tech_id, ", tech_level = ", tech_level);
			return { };
		}
		return boost::shared_ptr<const CastleTech>(tech_map, &*it);
	}
	boost::shared_ptr<const CastleTech> CastleTech::require(TechId tech_id, unsigned tech_level){
		PROFILE_ME;

		auto ret = get(tech_id, tech_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleTech not found: tech_id = ", tech_id, ", tech_level = ", tech_level);
			DEBUG_THROW(Exception, sslit("CastleTech not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleResource> CastleResource::get(ResourceId resource_id){
		PROFILE_ME;

		const auto resource_map = g_resource_map.lock();
		if(!resource_map){
			LOG_EMPERY_CENTER_WARNING("CastleResourceMap has not been loaded.");
			return { };
		}

		const auto it = resource_map->find<0>(resource_id);
		if(it == resource_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("CastleResource not found: resource_id = ", resource_id);
			return { };
		}
		return boost::shared_ptr<const CastleResource>(resource_map, &*it);
	}
	boost::shared_ptr<const CastleResource> CastleResource::require(ResourceId resource_id){
		PROFILE_ME;

		auto ret = get(resource_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleResource not found: resource_id = ", resource_id);
			DEBUG_THROW(Exception, sslit("CastleResource not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleResource> CastleResource::get_by_locked_resource_id(ResourceId locked_resource_id){
		PROFILE_ME;

		const auto resource_map = g_resource_map.lock();
		if(!resource_map){
			LOG_EMPERY_CENTER_WARNING("CastleResourceMap has not been loaded.");
			return { };
		}

		const auto it = resource_map->find<1>(locked_resource_id);
		if(it == resource_map->end<1>()){
			LOG_EMPERY_CENTER_TRACE("CastleResource not found: locked_resource_id = ", locked_resource_id);
			return { };
		}
		return boost::shared_ptr<const CastleResource>(resource_map, &*it);
	}
	boost::shared_ptr<const CastleResource> CastleResource::require_by_locked_resource_id(ResourceId locked_resource_id){
		PROFILE_ME;

		auto ret = get_by_locked_resource_id(locked_resource_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleResource not found: locked_resource_id = ", locked_resource_id);
			DEBUG_THROW(Exception, sslit("CastleResource not found"));
		}
		return ret;
	}

	void CastleResource::get_all(std::vector<boost::shared_ptr<const CastleResource>> &ret){
		PROFILE_ME;

		const auto resource_map = g_resource_map.lock();
		if(!resource_map){
			LOG_EMPERY_CENTER_WARNING("CastleResourceMap has not been loaded.");
			return;
		}

		ret.reserve(ret.size() + resource_map->size());
		for(auto it = resource_map->begin<0>(); it != resource_map->end<0>(); ++it){
			ret.emplace_back(resource_map, &*it);
		}
	}

	boost::shared_ptr<const CastleResource> CastleResource::get_by_carried_attribute_id(AttributeId attribute_id){
		PROFILE_ME;

		const auto resource_map = g_resource_map.lock();
		if(!resource_map){
			LOG_EMPERY_CENTER_WARNING("CastleResourceMap has not been loaded.");
			return { };
		}

		const auto it = resource_map->find<2>(attribute_id);
		if(it == resource_map->end<2>()){
			LOG_EMPERY_CENTER_TRACE("CastleResource not found: attribute_id = ", attribute_id);
			return { };
		}
		return boost::shared_ptr<const CastleResource>(resource_map, &*it);
	}

	void CastleResource::get_init(std::vector<boost::shared_ptr<const CastleResource>> &ret){
		PROFILE_ME;

		const auto resource_map = g_resource_map.lock();
		if(!resource_map){
			LOG_EMPERY_CENTER_WARNING("CastleResourceMap has not been loaded.");
			return;
		}

		const auto begin = resource_map->upper_bound<3>(0);
		const auto end = resource_map->end<3>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(resource_map, &*it);
		}
	}
	void CastleResource::get_auto_inc(std::vector<boost::shared_ptr<const CastleResource>> &ret){
		PROFILE_ME;

		const auto resource_map = g_resource_map.lock();
		if(!resource_map){
			LOG_EMPERY_CENTER_WARNING("CastleResourceMap has not been loaded.");
			return;
		}

		const auto begin = resource_map->upper_bound<4>(AIT_NONE);
		const auto end = resource_map->end<4>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(resource_map, &*it);
		}
	}
}

}
