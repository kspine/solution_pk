#include "../precompiled.hpp"
#include "castle.hpp"
#include <poseidon/multi_index_map.hpp>
#include "formats.hpp"
#include "../data_session.hpp"

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

	using CastleUpgradeBarracksMap = boost::container::flat_map<unsigned, Data::CastleUpgradeBarracks>;
	boost::weak_ptr<const CastleUpgradeBarracksMap> g_upgrade_barracks_map;
	const char UPGRADE_BARRACKS_FILE[] = "City_Camp";

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

	MULTI_INDEX_MAP(CastleTechMap, Data::CastleTech,
		UNIQUE_MEMBER_INDEX(tech_id_level)
	)
	boost::weak_ptr<const CastleTechMap> g_tech_map;
	const char TECH_FILE[] = "City_College_tech";

	template<typename ElementT>
	void read_upgrade_element(ElementT &elem, const Poseidon::CsvParser &csv){
		boost::uint64_t minutes;
		csv.get(minutes, "levelup_time");
		elem.upgrade_duration = checked_mul<boost::uint64_t>(minutes, 1000);//XXX

		std::string str;
		csv.get(str, "need_resource", "{}");
		try {
			std::istringstream iss(str);
			const auto root = Poseidon::JsonParser::parse_object(iss);
			elem.upgrade_cost.reserve(root.size());
			for(auto it = root.begin(); it != root.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto count = static_cast<boost::uint64_t>(it->second.get<double>());
				if(!elem.upgrade_cost.emplace(resource_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate upgrade cost: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate upgrade cost"));
				}
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", building_level = ", elem.building_level, ", str = ", str);
			throw;
		}

		csv.get(str, "need", "{}");
		try {
			std::istringstream iss(str);
			const auto root = Poseidon::JsonParser::parse_object(iss);
			elem.prerequisite.reserve(root.size());
			for(auto it = root.begin(); it != root.end(); ++it){
				const auto building_id = boost::lexical_cast<BuildingId>(it->first);
				const auto building_level = static_cast<unsigned>(it->second.get<double>());
				if(!elem.prerequisite.emplace(building_id, building_level).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate prerequisite: building_id = ", building_id);
					DEBUG_THROW(Exception, sslit("Duplicate prerequisite"));
				}
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", building_level = ", elem.building_level, ", str = ", str);
			throw;
		}

		csv.get(elem.prosperity_points, "prosperity");
		csv.get(minutes, "demolition");
		elem.destruct_duration = checked_mul<boost::uint64_t>(minutes, 10000);//XXX
	}

	MODULE_RAII_PRIORITY(handles, 1000){
		const auto data_directory = get_config<std::string>("data_directory", "empery_center_data");

		Poseidon::CsvParser csv;

		const auto building_base_map = boost::make_shared<CastleBuildingBaseMap>();
		auto path = data_directory + "/" + BUILDING_BASE_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading castle building bases: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::CastleBuildingBase elem = { };

			csv.get(elem.building_base_id, "index");

			std::string str;
			csv.get(str, "building_id", "[]");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parse_array(iss);
				elem.buildings_allowed.reserve(root.size());
				for(auto it = root.begin(); it != root.end(); ++it){
					const auto building_id = BuildingId(it->get<double>());
					if(!elem.buildings_allowed.emplace(building_id).second){
						LOG_EMPERY_CENTER_ERROR("Duplicate building allowed: building_id = ", building_id);
						DEBUG_THROW(Exception, sslit("Duplicate building allowed"));
					}
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", building_base_id = ", elem.building_base_id, ", str = ", str);
				throw;
			}

			csv.get(elem.init_level, "initial_level");

			if(!building_base_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate building base: building_base_id = ", elem.building_base_id);
				DEBUG_THROW(Exception, sslit("Duplicate building base"));
			}
		}
		g_building_base_map = building_base_map;
		handles.push(DataSession::create_servlet(BUILDING_BASE_FILE, serialize_csv(csv, "index")));
		handles.push(building_base_map);

		const auto building_map = boost::make_shared<CastleBuildingMap>();
		path = data_directory + "/" + BUILDING_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading castle buildings: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::CastleBuilding elem = { };

			csv.get(elem.building_id, "id");
			csv.get(elem.build_limit, "num");
			unsigned temp;
			csv.get(temp, "type");
			elem.type = static_cast<Data::CastleBuilding::Type>(temp);

			if(!building_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate building: building_id = ", elem.building_id);
				DEBUG_THROW(Exception, sslit("Duplicate building"));
			}
		}
		g_building_map = building_map;
		handles.push(DataSession::create_servlet(BUILDING_FILE, serialize_csv(csv, "id")));
		handles.push(building_map);

		const auto upgrade_primary_map = boost::make_shared<CastleUpgradePrimaryMap>();
		path = data_directory + "/" + UPGRADE_PRIMARY_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading castle upgrade primary: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::CastleUpgradePrimary elem = { };

			csv.get(elem.building_level, "castel_level");
			read_upgrade_element(elem, csv);

			if(!upgrade_primary_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade primary: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade primary"));
			}
		}
		g_upgrade_primary_map = upgrade_primary_map;
		handles.push(DataSession::create_servlet(UPGRADE_PRIMARY_FILE, serialize_csv(csv, "castel_level")));
		handles.push(upgrade_primary_map);

		const auto upgrade_barracks_map = boost::make_shared<CastleUpgradeBarracksMap>();
		path = data_directory + "/" + UPGRADE_BARRACKS_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading castle upgrade barracks: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::CastleUpgradeBarracks elem = { };

			csv.get(elem.building_level, "camp_level");
			read_upgrade_element(elem, csv);

			//

			if(!upgrade_barracks_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade barracks: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade barracks"));
			}
		}
		g_upgrade_barracks_map = upgrade_barracks_map;
		handles.push(DataSession::create_servlet(UPGRADE_BARRACKS_FILE, serialize_csv(csv, "camp_level")));
		handles.push(upgrade_barracks_map);

		const auto upgrade_academy_map = boost::make_shared<CastleUpgradeAcademyMap>();
		path = data_directory + "/" + UPGRADE_ACADEMY_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading castle upgrade academy: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::CastleUpgradeAcademy elem = { };

			csv.get(elem.building_level, "college_level");
			read_upgrade_element(elem, csv);

			csv.get(elem.tech_level, "tech_level");

			if(!upgrade_academy_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade academy: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade academy"));
			}
		}
		g_upgrade_academy_map = upgrade_academy_map;
		handles.push(DataSession::create_servlet(UPGRADE_ACADEMY_FILE, serialize_csv(csv, "college_level")));
		handles.push(upgrade_academy_map);

		const auto upgrade_civilian_map = boost::make_shared<CastleUpgradeCivilianMap>();
		path = data_directory + "/" + UPGRADE_CIVILIAN_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading castle upgrade civilian: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::CastleUpgradeCivilian elem = { };

			csv.get(elem.building_level, "house_level");
			read_upgrade_element(elem, csv);

			csv.get(elem.max_population, "population_max");

			if(!upgrade_civilian_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade civilian: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade civilian"));
			}
		}
		g_upgrade_civilian_map = upgrade_civilian_map;
		handles.push(DataSession::create_servlet(UPGRADE_CIVILIAN_FILE, serialize_csv(csv, "house_level")));
		handles.push(upgrade_civilian_map);

		const auto upgrade_warehouse_map = boost::make_shared<CastleUpgradeWarehouseMap>();
		path = data_directory + "/" + UPGRADE_WAREHOUSE_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading castle upgrade warehouse: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::CastleUpgradeWarehouse elem = { };

			csv.get(elem.building_level, "storage_level");
			read_upgrade_element(elem, csv);

			std::string str;
			csv.get(str, "resource_max", "{}");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parse_object(iss);
				elem.max_resource_amount.reserve(root.size());
				for(auto it = root.begin(); it != root.end(); ++it){
					const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
					const auto count = static_cast<boost::uint64_t>(it->second.get<double>());
					if(!elem.max_resource_amount.emplace(resource_id, count).second){
						LOG_EMPERY_CENTER_ERROR("Duplicate resource amount: resource_id = ", resource_id);
						DEBUG_THROW(Exception, sslit("Duplicate resource amount"));
					}
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", building_level = ", elem.building_level, ", str = ", str);
				throw;
			}

			if(!upgrade_warehouse_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade warehouse: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade warehouse"));
			}
		}
		g_upgrade_warehouse_map = upgrade_warehouse_map;
		handles.push(DataSession::create_servlet(UPGRADE_WAREHOUSE_FILE, serialize_csv(csv, "storage_level")));
		handles.push(upgrade_warehouse_map);

		const auto upgrade_citadel_wall_map = boost::make_shared<CastleUpgradeCitadelWallMap>();
		path = data_directory + "/" + UPGRADE_CITADEL_WALL_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading castle upgrade citadel wall: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::CastleUpgradeCitadelWall elem = { };

			csv.get(elem.building_level, "wall_level");
			read_upgrade_element(elem, csv);

			csv.get(elem.strength, "troops");
			csv.get(elem.armor, "defence");

			if(!upgrade_citadel_wall_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade citadel wall: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade citadel wall"));
			}
		}
		g_upgrade_citadel_wall_map = upgrade_citadel_wall_map;
		handles.push(DataSession::create_servlet(UPGRADE_CITADEL_WALL_FILE, serialize_csv(csv, "wall_level")));
		handles.push(upgrade_citadel_wall_map);

		const auto upgrade_defense_tower_map = boost::make_shared<CastleUpgradeDefenseTowerMap>();
		path = data_directory + "/" + UPGRADE_DEFENSE_TOWER_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading castle upgrade defense tower: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::CastleUpgradeDefenseTower elem = { };

			csv.get(elem.building_level, "tower_level");
			read_upgrade_element(elem, csv);

			csv.get(elem.firepower, "atk");

			if(!upgrade_defense_tower_map->emplace(elem.building_level, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate upgrade defense tower: building_level = ", elem.building_level);
				DEBUG_THROW(Exception, sslit("Duplicate upgrade defense tower"));
			}
		}
		g_upgrade_defense_tower_map = upgrade_defense_tower_map;
		handles.push(DataSession::create_servlet(UPGRADE_DEFENSE_TOWER_FILE, serialize_csv(csv, "tower_level")));
		handles.push(upgrade_defense_tower_map);

		const auto tech_map = boost::make_shared<CastleTechMap>();
		path = data_directory + "/" + TECH_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading castle tech: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::CastleTech elem = { };

			std::string str;
			csv.get(str, "tech_id_level");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parse_array(iss);
				elem.tech_id_level.first = TechId(root.at(0).get<double>());
				elem.tech_id_level.second = root.at(1).get<double>();
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", str = ", str);
				throw;
			}

			boost::uint64_t minutes;
			csv.get(minutes, "levelup_time");
			elem.upgrade_duration = checked_mul<boost::uint64_t>(minutes, 1000);//XXX

			csv.get(str, "need_resource", "{}");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parse_object(iss);
				elem.upgrade_cost.reserve(root.size());
				for(auto it = root.begin(); it != root.end(); ++it){
					const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
					const auto count = static_cast<boost::uint64_t>(it->second.get<double>());
					if(!elem.upgrade_cost.emplace(resource_id, count).second){
						LOG_EMPERY_CENTER_ERROR("Duplicate upgrade cost: resource_id = ", resource_id);
						DEBUG_THROW(Exception, sslit("Duplicate upgrade cost"));
					}
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(),
					", tech_id = ", elem.tech_id_level.first, ", level = ", elem.tech_id_level.second, ", str = ", str);
				throw;
			}

			csv.get(str, "need", "{}");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parse_object(iss);
				elem.prerequisite.reserve(root.size());
				for(auto it = root.begin(); it != root.end(); ++it){
					const auto building_id = boost::lexical_cast<BuildingId>(it->first);
					const auto building_level = static_cast<unsigned>(it->second.get<double>());
					if(!elem.prerequisite.emplace(building_id, building_level).second){
						LOG_EMPERY_CENTER_ERROR("Duplicate prerequisite: building_id = ", building_id);
						DEBUG_THROW(Exception, sslit("Duplicate prerequisite"));
					}
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(),
					", tech_id = ", elem.tech_id_level.first, ", level = ", elem.tech_id_level.second, ", str = ", str);
				throw;
			}

			csv.get(str, "level_open", "{}");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parse_object(iss);
				elem.display_prerequisite.reserve(root.size());
				for(auto it = root.begin(); it != root.end(); ++it){
					const auto building_id = boost::lexical_cast<BuildingId>(it->first);
					const auto building_level = static_cast<unsigned>(it->second.get<double>());
					if(!elem.display_prerequisite.emplace(building_id, building_level).second){
						LOG_EMPERY_CENTER_ERROR("Duplicate display prerequisite: building_id = ", building_id);
						DEBUG_THROW(Exception, sslit("Duplicate display prerequisite"));
					}
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(),
					", tech_id = ", elem.tech_id_level.first, ", level = ", elem.tech_id_level.second, ", str = ", str);
				throw;
			}

			csv.get(str, "tech_effect", "{}");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parse_object(iss);
				elem.attributes.reserve(root.size());
				for(auto it = root.begin(); it != root.end(); ++it){
					const auto attribute_id = boost::lexical_cast<AttributeId>(it->first);
					const auto value = it->second.get<double>();
					if(!elem.attributes.emplace(attribute_id, value).second){
						LOG_EMPERY_CENTER_ERROR("Duplicate attribute: attribute_id = ", attribute_id);
						DEBUG_THROW(Exception, sslit("Duplicate attribute"));
					}
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(),
					", tech_id = ", elem.tech_id_level.first, ", level = ", elem.tech_id_level.second, ", str = ", str);
				throw;
			}

			if(!tech_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate tech: tech_id = ", elem.tech_id_level.first, ", tech_level = ", elem.tech_id_level.second);
				DEBUG_THROW(Exception, sslit("Duplicate tech"));
			}
		}
		g_tech_map = tech_map;
		handles.push(DataSession::create_servlet(TECH_FILE, serialize_csv(csv, "tech_id_level")));
		handles.push(tech_map);
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
			LOG_EMPERY_CENTER_DEBUG("CastleBuildingBase not found: building_base_id = ", building_base_id);
			return { };
		}
		return boost::shared_ptr<const CastleBuildingBase>(building_base_map, &*it);
	}
	boost::shared_ptr<const CastleBuildingBase> CastleBuildingBase::require(BuildingBaseId building_base_id){
		PROFILE_ME;

		auto ret = get(building_base_id);
		if(!ret){
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
			LOG_EMPERY_CENTER_DEBUG("CastleBuilding not found: building_id = ", building_id);
			return { };
		}
		return boost::shared_ptr<const CastleBuilding>(building_map, &*it);
	}
	boost::shared_ptr<const CastleBuilding> CastleBuilding::require(BuildingId building_id){
		PROFILE_ME;

		auto ret = get(building_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleBuilding base not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeAbstract> CastleUpgradeAbstract::get(CastleBuilding::Type type, unsigned building_level){
		PROFILE_ME;

		switch(type){
		case CastleBuilding::T_PRIMARY:
			return CastleUpgradePrimary::get(building_level);
		case CastleBuilding::T_BARRACKS:
			return CastleUpgradeBarracks::get(building_level);
		case CastleBuilding::T_ACADEMY:
			return CastleUpgradeAcademy::get(building_level);
		case CastleBuilding::T_CIVILIAN:
			return CastleUpgradeCivilian::get(building_level);
//		case CastleBuilding::T_WATCHTOWER:
//			return CastleUpgradeWatchtower::get(building_level);
		case CastleBuilding::T_WAREHOUSE:
			return CastleUpgradeWarehouse::get(building_level);
		case CastleBuilding::T_CITADEL_WALL:
			return CastleUpgradeCitadelWall::get(building_level);
		case CastleBuilding::T_DEFENSE_TOWER:
			return CastleUpgradeDefenseTower::get(building_level);
		default:
			LOG_EMPERY_CENTER_WARNING("Unknown building type: type = ", static_cast<unsigned>(type));
			return { };
		}
	}
	boost::shared_ptr<const CastleUpgradeAbstract> CastleUpgradeAbstract::require(CastleBuilding::Type type, unsigned building_level){
		PROFILE_ME;

		auto ret = get(type, building_level);
		if(!ret){
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
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradePrimary not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradePrimary>(upgrade_primary_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradePrimary> CastleUpgradePrimary::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradePrimary not found"));
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
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeBarracks not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeBarracks>(upgrade_barracks_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeBarracks> CastleUpgradeBarracks::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradeBarracks not found"));
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
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeAcademy not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeAcademy>(upgrade_academy_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeAcademy> CastleUpgradeAcademy::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
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
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeCivilian not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeCivilian>(upgrade_civilian_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeCivilian> CastleUpgradeCivilian::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
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
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeWarehouse not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeWarehouse>(upgrade_warehouse_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeWarehouse> CastleUpgradeWarehouse::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
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
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeCitadelWall not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeCitadelWall>(upgrade_citadel_wall_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeCitadelWall> CastleUpgradeCitadelWall::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
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
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeDefenseTower not found: building_level = ", building_level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeDefenseTower>(upgrade_defense_tower_map, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeDefenseTower> CastleUpgradeDefenseTower::require(unsigned building_level){
		PROFILE_ME;

		auto ret = get(building_level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradeDefenseTower not found"));
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
			LOG_EMPERY_CENTER_DEBUG("CastleTech not found: tech_id = ", tech_id, ", tech_level = ", tech_level);
			return { };
		}
		return boost::shared_ptr<const CastleTech>(tech_map, &*it);
	}
	boost::shared_ptr<const CastleTech> CastleTech::require(TechId tech_id, unsigned tech_level){
		PROFILE_ME;

		auto ret = get(tech_id, tech_level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleTech not found"));
		}
		return ret;
	}
}

}
