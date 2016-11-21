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
	MULTI_INDEX_MAP(CastleBuildingBaseContainer, Data::CastleBuildingBase,
		UNIQUE_MEMBER_INDEX(building_base_id)
		MULTI_MEMBER_INDEX(init_level)
	)
	boost::weak_ptr<const CastleBuildingBaseContainer> g_building_base_container;
	const char BUILDING_BASE_FILE[] = "castle_block";

	MULTI_INDEX_MAP(CastleBuildingContainer, Data::CastleBuilding,
		UNIQUE_MEMBER_INDEX(building_id)
	)
	boost::weak_ptr<const CastleBuildingContainer> g_building_container;
	const char BUILDING_FILE[] = "castle_buildingxy";

	using CastleUpgradePrimaryContainer = boost::container::flat_map<unsigned, Data::CastleUpgradePrimary>;
	boost::weak_ptr<const CastleUpgradePrimaryContainer> g_upgrade_primary_container;
	const char UPGRADE_PRIMARY_FILE[] = "City_Castel";

	using CastleUpgradeStablesContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeStables>;
	boost::weak_ptr<const CastleUpgradeStablesContainer> g_upgrade_stables_container;
	const char UPGRADE_STABLES_FILE[] = "City_Camp_horse";

	using CastleUpgradeBarracksContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeBarracks>;
	boost::weak_ptr<const CastleUpgradeBarracksContainer> g_upgrade_barracks_container;
	const char UPGRADE_BARRACKS_FILE[] = "City_Camp_Barracks";

	using CastleUpgradeArcherBarracksContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeArcherBarracks>;
	boost::weak_ptr<const CastleUpgradeArcherBarracksContainer> g_upgrade_archer_barracks_container;
	const char UPGRADE_ARCHER_BARRACKS_FILE[] = "City_Camp_target";

	using CastleUpgradeWeaponryContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeWeaponry>;
	boost::weak_ptr<const CastleUpgradeWeaponryContainer> g_upgrade_weaponry_container;
	const char UPGRADE_WEAPONRY_FILE[] = "City_Camp_instrument";

	using CastleUpgradeAcademyContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeAcademy>;
	boost::weak_ptr<const CastleUpgradeAcademyContainer> g_upgrade_academy_container;
	const char UPGRADE_ACADEMY_FILE[] = "City_College";

	using CastleUpgradeCivilianContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeCivilian>;
	boost::weak_ptr<const CastleUpgradeCivilianContainer> g_upgrade_civilian_container;
	const char UPGRADE_CIVILIAN_FILE[] = "City_House";

	using CastleUpgradeWarehouseContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeWarehouse>;
	boost::weak_ptr<const CastleUpgradeWarehouseContainer> g_upgrade_warehouse_container;
	const char UPGRADE_WAREHOUSE_FILE[] = "City_Storage";

	using CastleUpgradeCitadelWallContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeCitadelWall>;
	boost::weak_ptr<const CastleUpgradeCitadelWallContainer> g_upgrade_citadel_wall_container;
	const char UPGRADE_CITADEL_WALL_FILE[] = "City_Wall";

	using CastleUpgradeDefenseTowerContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeDefenseTower>;
	boost::weak_ptr<const CastleUpgradeDefenseTowerContainer> g_upgrade_defense_tower_container;
	const char UPGRADE_DEFENSE_TOWER_FILE[] = "City_Tower";

	using CastleUpgradeParadeGroundContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeParadeGround>;
	boost::weak_ptr<const CastleUpgradeParadeGroundContainer> g_upgrade_parade_ground_container;
	const char UPGRADE_PARADE_GROUND_FILE[] = "City_Flied";

	using CastleUpgradeBootCampContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeBootCamp>;
	boost::weak_ptr<const CastleUpgradeBootCampContainer> g_upgrade_boot_camp_container;
	const char UPGRADE_BOOT_CAMP_FILE[] = "City_New_recruit";

	using CastleUpgradeMedicalTentContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeMedicalTent>;
	boost::weak_ptr<const CastleUpgradeMedicalTentContainer> g_upgrade_medical_tent_container;
	const char UPGRADE_MEDICAL_TENT_FILE[] = "City_Wounded_arm";

	using CastleUpgradeHarvestWorkshopContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeHarvestWorkshop>;
	boost::weak_ptr<const CastleUpgradeHarvestWorkshopContainer> g_upgrade_harvest_workshop_container;
	const char UPGRADE_HARVEST_WORKSHOP_FILE[] = "City_Collection";

	using CastleUpgradeWarWorkshopContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeWarWorkshop>;
	boost::weak_ptr<const CastleUpgradeWarWorkshopContainer> g_upgrade_war_workshop_container;
	const char UPGRADE_WAR_WORKSHOP_FILE[] = "City_Def";

	using CastleUpgradeTreeContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeTree>;
	boost::weak_ptr<const CastleUpgradeTreeContainer> g_upgrade_tree_container;
	const char UPGRADE_TREE_FILE[] = "City_Tree";

	using CastleUpgradeForgeContainer = boost::container::flat_map<unsigned, Data::CastleUpgradeForge>;
	boost::weak_ptr<const CastleUpgradeForgeContainer> g_upgrade_forge_container;
	const char UPGRADE_FORGE_FILE[] = "City_forge";

	MULTI_INDEX_MAP(CastleTechContainer, Data::CastleTech,
		UNIQUE_MEMBER_INDEX(tech_id_level)
		MULTI_MEMBER_INDEX(tech_era)
	)
	boost::weak_ptr<const CastleTechContainer> g_tech_container;
	const char TECH_FILE[] = "City_College_tech";

	MULTI_INDEX_MAP(CastleResourceContainer, Data::CastleResource,
		UNIQUE_MEMBER_INDEX(resource_id)
		MULTI_MEMBER_INDEX(locked_resource_id)
		MULTI_MEMBER_INDEX(carried_attribute_id)
		MULTI_MEMBER_INDEX(carried_alt_attribute_id)
		MULTI_MEMBER_INDEX(init_amount)
		MULTI_MEMBER_INDEX(auto_inc_type)
	)
	boost::weak_ptr<const CastleResourceContainer> g_resource_container;
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

	template<typename ElementT>
	void read_upgrade_addon_abstract(ElementT &elem, const Poseidon::CsvParser &csv){
		read_upgrade_abstract(elem, csv);

		Poseidon::JsonObject object;
		csv.get(object, "attributes");
		elem.attributes.reserve(object.size());
		for(auto it = object.begin(); it != object.end(); ++it){
			const auto attribute_id = boost::lexical_cast<AttributeId>(it->first);
			const auto value = it->second.get<double>();
			if(!elem.attributes.emplace(attribute_id, value).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate attribute: attribute_id = ", attribute_id);
				DEBUG_THROW(Exception, sslit("Duplicate attribute"));
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(BUILDING_BASE_FILE);
		const auto building_base_container = boost::make_shared<CastleBuildingBaseContainer>();
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

			if(!building_base_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate building base: building_base_id = ", elem.building_base_id);
				DEBUG_THROW(Exception, sslit("Duplicate building base"));
			}
		}
		g_building_base_container = building_base_container;
		handles.push(building_base_container);
		auto servlet = DataSession::create_servlet(BUILDING_BASE_FILE, Data::encode_csv_as_json(csv, "index"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(BUILDING_FILE);
		const auto building_container = boost::make_shared<CastleBuildingContainer>();
		while(csv.fetch_row()){
			Data::CastleBuilding elem = { };

			csv.get(elem.building_id, "id");
			csv.get(elem.build_limit, "num");
			csv.get(elem.type,        "type");

			if(!building_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate building: building_id = ", elem.building_id);
				DEBUG_THROW(Exception, sslit("Duplicate building"));
			}
		}
		g_building_container = building_container;
		handles.push(building_container);
		servlet = DataSession::create_servlet(BUILDING_FILE, Data::encode_csv_as_json(csv, "id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_PRIMARY_FILE);
		const auto upgrade_primary_container = boost::make_shared<CastleUpgradePrimaryContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradePrimary elem = { };

			csv.get(elem.building_level, "castel_level");
			read_upgrade_abstract(elem, csv);

			csv.get(elem.max_map_cell_count,        "territory_number");
			csv.get(elem.max_map_cell_distance,     "range");
			csv.get(elem.max_immigrant_group_count, "people");
			csv.get(elem.max_defense_buildings,     "towers_bunker_max");

			Poseidon::JsonObject object;
			csv.get(object, "need_fountain");
			elem.protection_cost.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto amount = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.protection_cost.emplace(resource_id, amount).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate resource: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate resource"));
				}
			}

			csv.get(elem.max_occupied_map_cells,    "max_occupy");

			if(!upgrade_primary_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradePrimary: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradePrimary"));
			}
		}
		g_upgrade_primary_container = upgrade_primary_container;
		handles.push(upgrade_primary_container);
		servlet = DataSession::create_servlet(UPGRADE_PRIMARY_FILE, Data::encode_csv_as_json(csv, "castel_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_STABLES_FILE);
		const auto upgrade_stables_container = boost::make_shared<CastleUpgradeStablesContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeStables elem = { };

			csv.get(elem.building_level, "camp_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_stables_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeStables: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeStables"));
			}
		}
		g_upgrade_stables_container = upgrade_stables_container;
		handles.push(upgrade_stables_container);
		servlet = DataSession::create_servlet(UPGRADE_STABLES_FILE, Data::encode_csv_as_json(csv, "camp_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_BARRACKS_FILE);
		const auto upgrade_barracks_container = boost::make_shared<CastleUpgradeBarracksContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeBarracks elem = { };

			csv.get(elem.building_level, "camp_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_barracks_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeBarracks: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeBarracks"));
			}
		}
		g_upgrade_barracks_container = upgrade_barracks_container;
		handles.push(upgrade_barracks_container);
		servlet = DataSession::create_servlet(UPGRADE_BARRACKS_FILE, Data::encode_csv_as_json(csv, "camp_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_ARCHER_BARRACKS_FILE);
		const auto upgrade_archer_barracks_container = boost::make_shared<CastleUpgradeArcherBarracksContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeArcherBarracks elem = { };

			csv.get(elem.building_level, "camp_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_archer_barracks_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeArcherBarracks: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeArcherBarracks"));
			}
		}
		g_upgrade_archer_barracks_container = upgrade_archer_barracks_container;
		handles.push(upgrade_archer_barracks_container);
		servlet = DataSession::create_servlet(UPGRADE_ARCHER_BARRACKS_FILE, Data::encode_csv_as_json(csv, "camp_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_WEAPONRY_FILE);
		const auto upgrade_weaponry_container = boost::make_shared<CastleUpgradeWeaponryContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeWeaponry elem = { };

			csv.get(elem.building_level, "camp_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_weaponry_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeWeaponry: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeWeaponry"));
			}
		}
		g_upgrade_weaponry_container = upgrade_weaponry_container;
		handles.push(upgrade_weaponry_container);
		servlet = DataSession::create_servlet(UPGRADE_WEAPONRY_FILE, Data::encode_csv_as_json(csv, "camp_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_ACADEMY_FILE);
		const auto upgrade_academy_container = boost::make_shared<CastleUpgradeAcademyContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeAcademy elem = { };

			csv.get(elem.building_level, "college_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_academy_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeAcademy: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeAcademy"));
			}
		}
		g_upgrade_academy_container = upgrade_academy_container;
		handles.push(upgrade_academy_container);
		servlet = DataSession::create_servlet(UPGRADE_ACADEMY_FILE, Data::encode_csv_as_json(csv, "college_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_CIVILIAN_FILE);
		const auto upgrade_civilian_container = boost::make_shared<CastleUpgradeCivilianContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeCivilian elem = { };

			csv.get(elem.building_level, "house_level");
			read_upgrade_abstract(elem, csv);

			csv.get(elem.population_production_rate, "population_produce");
			csv.get(elem.population_capacity,        "population_max");

			if(!upgrade_civilian_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeCivilian: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeCivilian"));
			}
		}
		g_upgrade_civilian_container = upgrade_civilian_container;
		handles.push(upgrade_civilian_container);
		servlet = DataSession::create_servlet(UPGRADE_CIVILIAN_FILE, Data::encode_csv_as_json(csv, "house_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_WAREHOUSE_FILE);
		const auto upgrade_warehouse_container = boost::make_shared<CastleUpgradeWarehouseContainer>();
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

			object.clear();
			csv.get(object, "resource_protect");
			elem.protected_resource_amounts.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.protected_resource_amounts.emplace(resource_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate resource: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate resource"));
				}
			}

			if(!upgrade_warehouse_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeWarehouse: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeWarehouse"));
			}
		}
		g_upgrade_warehouse_container = upgrade_warehouse_container;
		handles.push(upgrade_warehouse_container);
		servlet = DataSession::create_servlet(UPGRADE_WAREHOUSE_FILE, Data::encode_csv_as_json(csv, "storage_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_CITADEL_WALL_FILE);
		const auto upgrade_citadel_wall_container = boost::make_shared<CastleUpgradeCitadelWallContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeCitadelWall elem = { };

			csv.get(elem.building_level, "wall_level");
			read_upgrade_addon_abstract(elem, csv);

			//

			if(!upgrade_citadel_wall_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeCitadelWall: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeCitadelWall"));
			}
		}
		g_upgrade_citadel_wall_container = upgrade_citadel_wall_container;
		handles.push(upgrade_citadel_wall_container);
		servlet = DataSession::create_servlet(UPGRADE_CITADEL_WALL_FILE, Data::encode_csv_as_json(csv, "wall_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_DEFENSE_TOWER_FILE);
		const auto upgrade_defense_tower_container = boost::make_shared<CastleUpgradeDefenseTowerContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeDefenseTower elem = { };

			csv.get(elem.building_level, "tower_level");
			read_upgrade_addon_abstract(elem, csv);

			//

			if(!upgrade_defense_tower_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeDefenseTower: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeDefenseTower"));
			}
		}
		g_upgrade_defense_tower_container = upgrade_defense_tower_container;
		handles.push(upgrade_defense_tower_container);
		servlet = DataSession::create_servlet(UPGRADE_DEFENSE_TOWER_FILE, Data::encode_csv_as_json(csv, "tower_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_PARADE_GROUND_FILE);
		const auto upgrade_parade_ground_container = boost::make_shared<CastleUpgradeParadeGroundContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeParadeGround elem = { };

			csv.get(elem.building_level, "filed_level");
			read_upgrade_abstract(elem, csv);

			csv.get(elem.max_battalion_count,     "force_limit");
			csv.get(elem.max_soldier_count_bonus, "arm_max");

			if(!upgrade_parade_ground_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeParadeGround: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeParadeGround"));
			}
		}
		g_upgrade_parade_ground_container = upgrade_parade_ground_container;
		handles.push(upgrade_parade_ground_container);
		servlet = DataSession::create_servlet(UPGRADE_PARADE_GROUND_FILE, Data::encode_csv_as_json(csv, "filed_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_BOOT_CAMP_FILE);
		const auto upgrade_boot_camp_container = boost::make_shared<CastleUpgradeBootCampContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeBootCamp elem = { };

			csv.get(elem.building_level, "march_level");
			read_upgrade_addon_abstract(elem, csv);

			if(!upgrade_boot_camp_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeBootCamp: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeBootCamp"));
			}
		}
		g_upgrade_boot_camp_container = upgrade_boot_camp_container;
		handles.push(upgrade_boot_camp_container);
		servlet = DataSession::create_servlet(UPGRADE_BOOT_CAMP_FILE, Data::encode_csv_as_json(csv, "march_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_MEDICAL_TENT_FILE);
		const auto upgrade_medical_tent_container = boost::make_shared<CastleUpgradeMedicalTentContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeMedicalTent elem = { };

			csv.get(elem.building_level, "wounded_arm_level");
			read_upgrade_addon_abstract(elem, csv);

			csv.get(elem.capacity, "capacity");

			if(!upgrade_medical_tent_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeMedicalTent: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeMedicalTent"));
			}
		}
		g_upgrade_medical_tent_container = upgrade_medical_tent_container;
		handles.push(upgrade_medical_tent_container);
		servlet = DataSession::create_servlet(UPGRADE_MEDICAL_TENT_FILE, Data::encode_csv_as_json(csv, "wounded_arm_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_HARVEST_WORKSHOP_FILE);
		const auto upgrade_harvest_workshop_container = boost::make_shared<CastleUpgradeHarvestWorkshopContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeHarvestWorkshop elem = { };

			csv.get(elem.building_level, "collection_level");
			read_upgrade_addon_abstract(elem, csv);

			if(!upgrade_harvest_workshop_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeHarvestWorkshop: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeHarvestWorkshop"));
			}
		}
		g_upgrade_harvest_workshop_container = upgrade_harvest_workshop_container;
		handles.push(upgrade_harvest_workshop_container);
		servlet = DataSession::create_servlet(UPGRADE_HARVEST_WORKSHOP_FILE, Data::encode_csv_as_json(csv, "collection_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_WAR_WORKSHOP_FILE);
		const auto upgrade_war_workshop_container = boost::make_shared<CastleUpgradeWarWorkshopContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeWarWorkshop elem = { };

			csv.get(elem.building_level, "city_def_level");
			read_upgrade_addon_abstract(elem, csv);

			if(!upgrade_war_workshop_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeWarWorkshop: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeWarWorkshop"));
			}
		}
		g_upgrade_war_workshop_container = upgrade_war_workshop_container;
		handles.push(upgrade_war_workshop_container);
		servlet = DataSession::create_servlet(UPGRADE_WAR_WORKSHOP_FILE, Data::encode_csv_as_json(csv, "city_def_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_TREE_FILE);
		const auto upgrade_tree_container = boost::make_shared<CastleUpgradeTreeContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeTree elem = { };

			csv.get(elem.building_level, "tree_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_tree_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeTree: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeTree"));
			}
		}
		g_upgrade_tree_container = upgrade_tree_container;
		handles.push(upgrade_tree_container);
		servlet = DataSession::create_servlet(UPGRADE_TREE_FILE, Data::encode_csv_as_json(csv, "tree_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(UPGRADE_FORGE_FILE);
		const auto upgrade_forge_container = boost::make_shared<CastleUpgradeForgeContainer>();
		while(csv.fetch_row()){
			Data::CastleUpgradeForge elem = { };

			csv.get(elem.building_level, "forge_level");
			read_upgrade_abstract(elem, csv);

			//

			if(!upgrade_forge_container->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate CastleUpgradeForge: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate CastleUpgradeForge"));
			}
		}
		g_upgrade_forge_container = upgrade_forge_container;
		handles.push(upgrade_forge_container);
		servlet = DataSession::create_servlet(UPGRADE_FORGE_FILE, Data::encode_csv_as_json(csv, "forge_level"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(TECH_FILE);
		const auto tech_container = boost::make_shared<CastleTechContainer>();
		while(csv.fetch_row()){
			Data::CastleTech elem = { };

			Poseidon::JsonArray array;
			csv.get(array, "tech_level_id");
			elem.tech_id_level.first = TechId(array.at(0).get<double>());
			elem.tech_id_level.second = array.at(1).get<double>();

			csv.get(elem.tech_era,         "tech_class");
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
				const auto level = static_cast<unsigned>(it->second.get<double>());
				if(!elem.prerequisite.emplace(building_id, level).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate prerequisite: building_id = ", building_id);
					DEBUG_THROW(Exception, sslit("Duplicate prerequisite"));
				}
			}

			object.clear();
			csv.get(object, "level_open");
			elem.display_prerequisite.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto building_id = boost::lexical_cast<BuildingId>(it->first);
				const auto level = static_cast<unsigned>(it->second.get<double>());
				if(!elem.display_prerequisite.emplace(building_id, level).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate display prerequisite: building_id = ", building_id);
					DEBUG_THROW(Exception, sslit("Duplicate display prerequisite"));
				}
			}

			object.clear();
			csv.get(object, "level_need");
			elem.tech_prerequisite.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto tech_id = boost::lexical_cast<TechId>(it->first);
				const auto level = static_cast<unsigned>(it->second.get<double>());
				if(!elem.tech_prerequisite.emplace(tech_id, level).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate tech prerequisite: tech_id = ", tech_id);
					DEBUG_THROW(Exception, sslit("Duplicate tech prerequisite"));
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

			if(!tech_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate tech: tech_id = ", elem.tech_id_level.first, ", tech_level = ", elem.tech_id_level.second);
				DEBUG_THROW(Exception, sslit("Duplicate tech"));
			}
		}
		g_tech_container = tech_container;
		handles.push(tech_container);
		servlet = DataSession::create_servlet(TECH_FILE, Data::encode_csv_as_json(csv, "tech_level_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(RESOURCE_FILE);
		const auto resource_container = boost::make_shared<CastleResourceContainer>();
		while(csv.fetch_row()){
			Data::CastleResource elem = { };

			csv.get(elem.resource_id,          "material_id");

			csv.get(elem.init_amount,          "number");
			csv.get(elem.producible,           "producible");

			csv.get(elem.locked_resource_id,   "lock_material_id");
			csv.get(elem.undeployed_item_id,   "item_id");

			csv.get(elem.carried_attribute_id,     "weight_id");
			csv.get(elem.carried_alt_attribute_id, "system_id");
			csv.get(elem.unit_weight,              "weight");

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

			csv.get(elem.resource_token_id,    "trade");

			if(!resource_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate initial resource: resource_id = ", elem.resource_id,
					", locked_resource_id = ", elem.locked_resource_id);
				DEBUG_THROW(Exception, sslit("Duplicate initial resource"));
			}
		}
		g_resource_container = resource_container;
		handles.push(resource_container);
		servlet = DataSession::create_servlet(RESOURCE_FILE, Data::encode_csv_as_json(csv, "material_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const CastleBuildingBase> CastleBuildingBase::get(BuildingBaseId building_base_id){
		PROFILE_ME;

		const auto building_base_container = g_building_base_container.lock();
		if(!building_base_container){
			LOG_EMPERY_CENTER_WARNING("CastleBuildingBaseContainer has not been loaded.");
			return { };
		}

		const auto it = building_base_container->find<0>(building_base_id);
		if(it == building_base_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("CastleBuildingBase not found: building_base_id = ", building_base_id);
			return { };
		}
		return boost::shared_ptr<const CastleBuildingBase>(building_base_container, &*it);
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

		const auto building_base_container = g_building_base_container.lock();
		if(!building_base_container){
			LOG_EMPERY_CENTER_WARNING("CastleBuildingBaseContainer has not been loaded.");
			return;
		}

		const auto range = std::make_pair(building_base_container->upper_bound<1>(0), building_base_container->end<1>());
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			ret.emplace_back(building_base_container, &*it);
		}
	}

	boost::shared_ptr<const CastleBuilding> CastleBuilding::get(BuildingId building_id){
		PROFILE_ME;

		const auto building_container = g_building_container.lock();
		if(!building_container){
			LOG_EMPERY_CENTER_WARNING("CastleBuildingContainer has not been loaded.");
			return { };
		}

		const auto it = building_container->find<0>(building_id);
		if(it == building_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("CastleBuilding not found: building_id = ", building_id);
			return { };
		}
		return boost::shared_ptr<const CastleBuilding>(building_container, &*it);
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
		case BuildingTypeIds::ID_FORGE.get():
			return CastleUpgradeForge::get(building_level);
		default:
			LOG_EMPERY_CENTER_TRACE("Unhandled building type: type = ", type);
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

		const auto upgrade_primary_container = g_upgrade_primary_container.lock();
		if(!upgrade_primary_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradePrimaryContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_primary_container->find(building_level);
		if(it == upgrade_primary_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradePrimary not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradePrimary>(upgrade_primary_container, &(it->second));
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

		const auto upgrade_stables_container = g_upgrade_stables_container.lock();
		if(!upgrade_stables_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeStablesContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_stables_container->find(building_level);
		if(it == upgrade_stables_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeStables not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeStables>(upgrade_stables_container, &(it->second));
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

		const auto upgrade_barracks_container = g_upgrade_barracks_container.lock();
		if(!upgrade_barracks_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeBarracksContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_barracks_container->find(building_level);
		if(it == upgrade_barracks_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeBarracks not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeBarracks>(upgrade_barracks_container, &(it->second));
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

		const auto upgrade_archer_barracks_container = g_upgrade_archer_barracks_container.lock();
		if(!upgrade_archer_barracks_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeArcherBarracksContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_archer_barracks_container->find(building_level);
		if(it == upgrade_archer_barracks_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeArcherBarracks not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeArcherBarracks>(upgrade_archer_barracks_container, &(it->second));
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

		const auto upgrade_weaponry_container = g_upgrade_weaponry_container.lock();
		if(!upgrade_weaponry_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeWeaponryContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_weaponry_container->find(building_level);
		if(it == upgrade_weaponry_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeWeaponry not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeWeaponry>(upgrade_weaponry_container, &(it->second));
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

		const auto upgrade_academy_container = g_upgrade_academy_container.lock();
		if(!upgrade_academy_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeAcademyContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_academy_container->find(building_level);
		if(it == upgrade_academy_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeAcademy not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeAcademy>(upgrade_academy_container, &(it->second));
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

		const auto upgrade_civilian_container = g_upgrade_civilian_container.lock();
		if(!upgrade_civilian_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeCivilianContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_civilian_container->find(building_level);
		if(it == upgrade_civilian_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeCivilian not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeCivilian>(upgrade_civilian_container, &(it->second));
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

		const auto upgrade_warehouse_container = g_upgrade_warehouse_container.lock();
		if(!upgrade_warehouse_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeWarehouseContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_warehouse_container->find(building_level);
		if(it == upgrade_warehouse_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeWarehouse not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeWarehouse>(upgrade_warehouse_container, &(it->second));
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

		const auto upgrade_citadel_wall_container = g_upgrade_citadel_wall_container.lock();
		if(!upgrade_citadel_wall_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeCitadelWallContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_citadel_wall_container->find(building_level);
		if(it == upgrade_citadel_wall_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeCitadelWall not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeCitadelWall>(upgrade_citadel_wall_container, &(it->second));
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

		const auto upgrade_defense_tower_container = g_upgrade_defense_tower_container.lock();
		if(!upgrade_defense_tower_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeDefenseTowerContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_defense_tower_container->find(building_level);
		if(it == upgrade_defense_tower_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeDefenseTower not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeDefenseTower>(upgrade_defense_tower_container, &(it->second));
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

		const auto upgrade_parade_ground_container = g_upgrade_parade_ground_container.lock();
		if(!upgrade_parade_ground_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeParadeGroundContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_parade_ground_container->find(building_level);
		if(it == upgrade_parade_ground_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeParadeGround not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeParadeGround>(upgrade_parade_ground_container, &(it->second));
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

	boost::shared_ptr<const CastleUpgradeAddonAbstract> CastleUpgradeAddonAbstract::get(BuildingTypeId type, unsigned building_level){
		PROFILE_ME;

		switch(type.get()){
		case BuildingTypeIds::ID_CITADEL_WALL.get():
			return CastleUpgradeCitadelWall::get(building_level);
		case BuildingTypeIds::ID_DEFENSE_TOWER.get():
			return CastleUpgradeDefenseTower::get(building_level);
		case BuildingTypeIds::ID_BOOT_CAMP.get():
			return CastleUpgradeBootCamp::get(building_level);
		case BuildingTypeIds::ID_MEDICAL_TENT.get():
			return CastleUpgradeMedicalTent::get(building_level);
		case BuildingTypeIds::ID_HARVEST_WORKSHOP.get():
			return CastleUpgradeHarvestWorkshop::get(building_level);
		case BuildingTypeIds::ID_WAR_WORKSHOP.get():
			return CastleUpgradeWarWorkshop::get(building_level);
		default:
			LOG_EMPERY_CENTER_TRACE("Unhandled building type: type = ", type);
			return { };
		}
	}
	boost::shared_ptr<const CastleUpgradeAddonAbstract> CastleUpgradeAddonAbstract::require(BuildingTypeId type, unsigned building_level){
		PROFILE_ME;

		auto ret = get(type, building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeAddonAbstract not found: type = ", type, ", building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeAddonAbstract not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeBootCamp> CastleUpgradeBootCamp::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_boot_camp_container = g_upgrade_boot_camp_container.lock();
		if(!upgrade_boot_camp_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeBootCampContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_boot_camp_container->find(building_level);
		if(it == upgrade_boot_camp_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeBootCamp not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeBootCamp>(upgrade_boot_camp_container, &(it->second));
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

		const auto upgrade_medical_tent_container = g_upgrade_medical_tent_container.lock();
		if(!upgrade_medical_tent_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeMedicalTentContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_medical_tent_container->find(building_level);
		if(it == upgrade_medical_tent_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeMedicalTent not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeMedicalTent>(upgrade_medical_tent_container, &(it->second));
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

		const auto upgrade_harvest_workshop_container = g_upgrade_harvest_workshop_container.lock();
		if(!upgrade_harvest_workshop_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeHarvestWorkshopContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_harvest_workshop_container->find(building_level);
		if(it == upgrade_harvest_workshop_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeHarvestWorkshop not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeHarvestWorkshop>(upgrade_harvest_workshop_container, &(it->second));
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

		const auto upgrade_war_workshop_container = g_upgrade_war_workshop_container.lock();
		if(!upgrade_war_workshop_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeWarWorkshopContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_war_workshop_container->find(building_level);
		if(it == upgrade_war_workshop_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeWarWorkshop not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeWarWorkshop>(upgrade_war_workshop_container, &(it->second));
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

		const auto upgrade_tree_container = g_upgrade_tree_container.lock();
		if(!upgrade_tree_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeTreeContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_tree_container->find(building_level);
		if(it == upgrade_tree_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeTree not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeTree>(upgrade_tree_container, &(it->second));
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

	boost::shared_ptr<const CastleUpgradeForge> CastleUpgradeForge::get(unsigned building_level){
		PROFILE_ME;

		const auto upgrade_forge_container = g_upgrade_forge_container.lock();
		if(!upgrade_forge_container){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeForgeContainer has not been loaded.");
			return { };
		}

		const auto it = upgrade_forge_container->find(building_level);
		if(it == upgrade_forge_container->end()){
			LOG_EMPERY_CENTER_TRACE("CastleUpgradeForge not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeForge>(upgrade_forge_container, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeForge> CastleUpgradeForge::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeForge not found: building_level = ", building_level);
			DEBUG_THROW(Exception, sslit("CastleUpgradeForge not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleTech> CastleTech::get(TechId tech_id, unsigned tech_level){
		PROFILE_ME;

		const auto tech_container = g_tech_container.lock();
		if(!tech_container){
			LOG_EMPERY_CENTER_WARNING("CastleTechContainer has not been loaded.");
			return { };
		}

		const auto it = tech_container->find<0>(std::make_pair(tech_id, tech_level));
		if(it == tech_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("CastleTech not found: tech_id = ", tech_id, ", tech_level = ", tech_level);
			return { };
		}
		return boost::shared_ptr<const CastleTech>(tech_container, &*it);
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

	void CastleTech::get_by_era(std::vector<boost::shared_ptr<const CastleTech>> &ret, unsigned era){
		PROFILE_ME;

		const auto tech_container = g_tech_container.lock();
		if(!tech_container){
			LOG_EMPERY_CENTER_WARNING("CastleTechContainer has not been loaded.");
			return;
		}

		const auto range = tech_container->equal_range<1>(era);
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			ret.emplace_back(tech_container, &*it);
		}
	}

	boost::shared_ptr<const CastleResource> CastleResource::get(ResourceId resource_id){
		PROFILE_ME;

		const auto resource_container = g_resource_container.lock();
		if(!resource_container){
			LOG_EMPERY_CENTER_WARNING("CastleResourceContainer has not been loaded.");
			return { };
		}

		const auto it = resource_container->find<0>(resource_id);
		if(it == resource_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("CastleResource not found: resource_id = ", resource_id);
			return { };
		}
		return boost::shared_ptr<const CastleResource>(resource_container, &*it);
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

		const auto resource_container = g_resource_container.lock();
		if(!resource_container){
			LOG_EMPERY_CENTER_WARNING("CastleResourceContainer has not been loaded.");
			return { };
		}

		const auto it = resource_container->find<1>(locked_resource_id);
		if(it == resource_container->end<1>()){
			LOG_EMPERY_CENTER_TRACE("CastleResource not found: locked_resource_id = ", locked_resource_id);
			return { };
		}
		return boost::shared_ptr<const CastleResource>(resource_container, &*it);
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

		const auto resource_container = g_resource_container.lock();
		if(!resource_container){
			LOG_EMPERY_CENTER_WARNING("CastleResourceContainer has not been loaded.");
			return;
		}

		ret.reserve(ret.size() + resource_container->size());
		for(auto it = resource_container->begin<0>(); it != resource_container->end<0>(); ++it){
			ret.emplace_back(resource_container, &*it);
		}
	}

	boost::shared_ptr<const CastleResource> CastleResource::get_by_carried_attribute_id(AttributeId attribute_id){
		PROFILE_ME;

		const auto resource_container = g_resource_container.lock();
		if(!resource_container){
			LOG_EMPERY_CENTER_WARNING("CastleResourceContainer has not been loaded.");
			return { };
		}

		const auto it = resource_container->find<2>(attribute_id);
		if(it != resource_container->end<2>()){
			return boost::shared_ptr<const CastleResource>(resource_container, &*it);
		}
		const auto alt_it = resource_container->find<3>(attribute_id);
		if(alt_it != resource_container->end<3>()){
			return boost::shared_ptr<const CastleResource>(resource_container, &*alt_it);
		}
		LOG_EMPERY_CENTER_TRACE("CastleResource not found: attribute_id = ", attribute_id);
		return { };
	}

	void CastleResource::get_init(std::vector<boost::shared_ptr<const CastleResource>> &ret){
		PROFILE_ME;

		const auto resource_container = g_resource_container.lock();
		if(!resource_container){
			LOG_EMPERY_CENTER_WARNING("CastleResourceContainer has not been loaded.");
			return;
		}

		const auto begin = resource_container->upper_bound<4>(0);
		const auto end = resource_container->end<4>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(resource_container, &*it);
		}
	}
	void CastleResource::get_auto_inc(std::vector<boost::shared_ptr<const CastleResource>> &ret){
		PROFILE_ME;

		const auto resource_container = g_resource_container.lock();
		if(!resource_container){
			LOG_EMPERY_CENTER_WARNING("CastleResourceContainer has not been loaded.");
			return;
		}

		const auto begin = resource_container->upper_bound<5>(AIT_NONE);
		const auto end = resource_container->end<5>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(resource_container, &*it);
		}
	}
}

}
