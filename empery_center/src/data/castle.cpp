#include "../precompiled.hpp"
#include "castle.hpp"
#include <poseidon/multi_index_map.hpp>
#include "formats.hpp"
#include "../data_session.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(CastleBuildingBaseMap, Data::CastleBuildingBase,
		UNIQUE_MEMBER_INDEX(baseIndex)
	)
	boost::weak_ptr<const CastleBuildingBaseMap> g_buildingBaseMap;
	const char BUILDING_BASE_FILE[] = "castle_block";

	MULTI_INDEX_MAP(CastleBuildingMap, Data::CastleBuilding,
		UNIQUE_MEMBER_INDEX(buildingId)
	)
	boost::weak_ptr<const CastleBuildingMap> g_buildingMap;
	const char BUILDING_FILE[] = "castle_buildingxy";

	MULTI_INDEX_MAP(CastleUpgradingPrimaryMap, Data::CastleUpgradingPrimary,
		UNIQUE_MEMBER_INDEX(level)
	)
	boost::weak_ptr<const CastleUpgradingPrimaryMap> g_upgradingPrimaryMap;
	const char UPGRADING_PRIMARY_FILE[] = "City_Castel";

	MULTI_INDEX_MAP(CastleUpgradingBarracksMap, Data::CastleUpgradingBarracks,
		UNIQUE_MEMBER_INDEX(level)
	)
	boost::weak_ptr<const CastleUpgradingBarracksMap> g_upgradingBarracksMap;
	const char UPGRADING_BARRACKS_FILE[] = "City_Camp";

	MULTI_INDEX_MAP(CastleUpgradingAcademyMap, Data::CastleUpgradingAcademy,
		UNIQUE_MEMBER_INDEX(level)
	)
	boost::weak_ptr<const CastleUpgradingAcademyMap> g_upgradingAcademyMap;
	const char UPGRADING_ACADEMY_FILE[] = "City_College";

	MULTI_INDEX_MAP(CastleUpgradingCivilianMap, Data::CastleUpgradingCivilian,
		UNIQUE_MEMBER_INDEX(level)
	)
	boost::weak_ptr<const CastleUpgradingCivilianMap> g_upgradingCivilianMap;
	const char UPGRADING_CIVILIAN_FILE[] = "City_House";

	MULTI_INDEX_MAP(CastleUpgradingWarehouseMap, Data::CastleUpgradingWarehouse,
		UNIQUE_MEMBER_INDEX(level)
	)
	boost::weak_ptr<const CastleUpgradingWarehouseMap> g_upgradingWarehouseMap;
	const char UPGRADING_WAREHOUSE_FILE[] = "City_Storage";

	MULTI_INDEX_MAP(CastleUpgradingCitadelWallMap, Data::CastleUpgradingCitadelWall,
		UNIQUE_MEMBER_INDEX(level)
	)
	boost::weak_ptr<const CastleUpgradingCitadelWallMap> g_upgradingCitadelWallMap;
	const char UPGRADING_CITADEL_WALL_FILE[] = "City_Wall";

	MULTI_INDEX_MAP(CastleUpgradingDefenseTowerMap, Data::CastleUpgradingDefenseTower,
		UNIQUE_MEMBER_INDEX(level)
	)
	boost::weak_ptr<const CastleUpgradingDefenseTowerMap> g_upgradingDefenseTowerMap;
	const char UPGRADING_DEFENSE_TOWER_FILE[] = "City_Tower";

	template<typename ElementT>
	void readUpgradingElement(ElementT &elem, const Poseidon::CsvParser &csv){
		unsigned temp;
		csv.get(temp, "levelup_time");
		elem.upgradeDuration = checkedMul<boost::uint64_t>(temp, 60000);

		std::string str;
		csv.get(str, "need_resource", "{}");
		try {
			std::istringstream iss(str);
			const auto root = Poseidon::JsonParser::parseObject(iss);
			elem.upgradeCost.reserve(root.size());
			for(auto it = root.begin(); it != root.end(); ++it){
				const auto resourceId = boost::lexical_cast<ResourceId>(it->first);
				const auto count = static_cast<boost::uint64_t>(it->second.get<double>());
				if(!elem.upgradeCost.emplace(resourceId, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate upgrade cost: resourceId = ", resourceId);
					DEBUG_THROW(Exception, sslit("Duplicate upgrade cost"));
				}
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", level = ", elem.level, ", str = ", str);
			throw;
		}

		csv.get(str, "need", "{}");
		try {
			std::istringstream iss(str);
			const auto root = Poseidon::JsonParser::parseObject(iss);
			elem.prerequisite.reserve(root.size());
			for(auto it = root.begin(); it != root.end(); ++it){
				const auto buildingId = boost::lexical_cast<BuildingId>(it->first);
				const auto level = static_cast<unsigned>(it->second.get<double>());
				if(!elem.prerequisite.emplace(buildingId, level).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate prerequisite: buildingId = ", buildingId);
					DEBUG_THROW(Exception, sslit("Duplicate prerequisite"));
				}
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", level = ", elem.level, ", str = ", str);
			throw;
		}
	}
}

MODULE_RAII_PRIORITY(handles, 1000){
	const auto dataDirectory = getConfig<std::string>("data_directory", "empery_center_data");

	Poseidon::CsvParser csv;

	const auto buildingBaseMap = boost::make_shared<CastleBuildingBaseMap>();
	auto path = dataDirectory + "/" + BUILDING_BASE_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle building bases: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleBuildingBase elem = { };

		csv.get(elem.baseIndex, "index");

		std::string str;
		csv.get(str, "building_id", "[]");
		try {
			std::istringstream iss(str);
			const auto root = Poseidon::JsonParser::parseArray(iss);
			elem.buildingsAllowed.reserve(root.size());
			for(auto it = root.begin(); it != root.end(); ++it){
				const auto buildingId = BuildingId(it->get<double>());
				if(!elem.buildingsAllowed.emplace(buildingId).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate building allowed: buildingId = ", buildingId);
					DEBUG_THROW(Exception, sslit("Duplicate building allowed"));
				}
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", baseIndex = ", elem.baseIndex, ", str = ", str);
			throw;
		}

		if(!buildingBaseMap->insert(std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate building base: baseIndex = ", elem.baseIndex);
			DEBUG_THROW(Exception, sslit("Duplicate building base"));
		}
	}
	g_buildingBaseMap = buildingBaseMap;
	handles.push(DataSession::createServlet(BUILDING_BASE_FILE, serializeCsv(csv, "index")));
	handles.push(buildingBaseMap);

	const auto buildingMap = boost::make_shared<CastleBuildingMap>();
	path = dataDirectory + "/" + BUILDING_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle buildings: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleBuilding elem = { };

		unsigned temp;
		csv.get(elem.buildingId, "id");
		csv.get(elem.buildLimit, "num");
		csv.get(temp, "type");
		elem.type = static_cast<Data::CastleBuilding::Type>(temp);
		csv.get(elem.destructible, "Demolition");

		if(!buildingMap->insert(std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate building: buildingId = ", elem.buildingId);
			DEBUG_THROW(Exception, sslit("Duplicate building"));
		}
	}
	g_buildingMap = buildingMap;
	handles.push(DataSession::createServlet(BUILDING_FILE, serializeCsv(csv, "id")));
	handles.push(buildingMap);

	const auto upgradingPrimaryMap = boost::make_shared<CastleUpgradingPrimaryMap>();
	path = dataDirectory + "/" + UPGRADING_PRIMARY_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrading primary: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradingPrimary elem = { };

		csv.get(elem.level, "castel_level");
		readUpgradingElement(elem, csv);

		if(!upgradingPrimaryMap->insert(std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrading primary: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrading primary"));
		}
	}
	g_upgradingPrimaryMap = upgradingPrimaryMap;
	handles.push(DataSession::createServlet(UPGRADING_PRIMARY_FILE, serializeCsv(csv, "castel_level")));
	handles.push(upgradingPrimaryMap);

	const auto upgradingBarracksMap = boost::make_shared<CastleUpgradingBarracksMap>();
	path = dataDirectory + "/" + UPGRADING_BARRACKS_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrading barracks: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradingBarracks elem = { };

		csv.get(elem.level, "camp_level");
		readUpgradingElement(elem, csv);

		//

		if(!upgradingBarracksMap->insert(std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrading barracks: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrading barracks"));
		}
	}
	g_upgradingBarracksMap = upgradingBarracksMap;
	handles.push(DataSession::createServlet(UPGRADING_BARRACKS_FILE, serializeCsv(csv, "camp_level")));
	handles.push(upgradingBarracksMap);

	const auto upgradingAcademyMap = boost::make_shared<CastleUpgradingAcademyMap>();
	path = dataDirectory + "/" + UPGRADING_ACADEMY_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrading academy: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradingAcademy elem = { };

		csv.get(elem.level, "college_level");
		readUpgradingElement(elem, csv);

		csv.get(elem.techLevel, "tech_level");

		if(!upgradingAcademyMap->insert(std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrading academy: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrading academy"));
		}
	}
	g_upgradingAcademyMap = upgradingAcademyMap;
	handles.push(DataSession::createServlet(UPGRADING_ACADEMY_FILE, serializeCsv(csv, "college_level")));
	handles.push(upgradingAcademyMap);

	const auto upgradingCivilianMap = boost::make_shared<CastleUpgradingCivilianMap>();
	path = dataDirectory + "/" + UPGRADING_CIVILIAN_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrading civilian: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradingCivilian elem = { };

		csv.get(elem.level, "house_level");
		readUpgradingElement(elem, csv);

		csv.get(elem.maxPopulation, "population_max");

		if(!upgradingCivilianMap->insert(std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrading civilian: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrading civilian"));
		}
	}
	g_upgradingCivilianMap = upgradingCivilianMap;
	handles.push(DataSession::createServlet(UPGRADING_CIVILIAN_FILE, serializeCsv(csv, "house_level")));
	handles.push(upgradingCivilianMap);

	const auto upgradingWarehouseMap = boost::make_shared<CastleUpgradingWarehouseMap>();
	path = dataDirectory + "/" + UPGRADING_WAREHOUSE_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrading warehouse: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradingWarehouse elem = { };

		csv.get(elem.level, "storage_level");
		readUpgradingElement(elem, csv);

		std::string str;
		csv.get(str, "resource_max", "{}");
		try {
			std::istringstream iss(str);
			const auto root = Poseidon::JsonParser::parseObject(iss);
			elem.maxResourceAmount.reserve(root.size());
			for(auto it = root.begin(); it != root.end(); ++it){
				const auto resourceId = boost::lexical_cast<ResourceId>(it->first);
				const auto count = static_cast<boost::uint64_t>(it->second.get<double>());
				if(!elem.maxResourceAmount.emplace(resourceId, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate resource amount: resourceId = ", resourceId);
					DEBUG_THROW(Exception, sslit("Duplicate resource amount"));
				}
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", level = ", elem.level, ", str = ", str);
			throw;
		}

		if(!upgradingWarehouseMap->insert(std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrading warehouse: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrading warehouse"));
		}
	}
	g_upgradingWarehouseMap = upgradingWarehouseMap;
	handles.push(DataSession::createServlet(UPGRADING_WAREHOUSE_FILE, serializeCsv(csv, "storage_level")));
	handles.push(upgradingWarehouseMap);

	const auto upgradingCitadelWallMap = boost::make_shared<CastleUpgradingCitadelWallMap>();
	path = dataDirectory + "/" + UPGRADING_CITADEL_WALL_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrading citadel wall: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradingCitadelWall elem = { };

		csv.get(elem.level, "wall_level");
		readUpgradingElement(elem, csv);

		csv.get(elem.strength, "troops");
		csv.get(elem.armor, "defence");

		if(!upgradingCitadelWallMap->insert(std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrading citadel wall: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrading citadel wall"));
		}
	}
	g_upgradingCitadelWallMap = upgradingCitadelWallMap;
	handles.push(DataSession::createServlet(UPGRADING_CITADEL_WALL_FILE, serializeCsv(csv, "wall_level")));
	handles.push(upgradingCitadelWallMap);

	const auto upgradingDefenseTowerMap = boost::make_shared<CastleUpgradingDefenseTowerMap>();
	path = dataDirectory + "/" + UPGRADING_DEFENSE_TOWER_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrading defense tower: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradingDefenseTower elem = { };

		csv.get(elem.level, "tower_level");
		readUpgradingElement(elem, csv);

		csv.get(elem.firepower, "atk");

		if(!upgradingDefenseTowerMap->insert(std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrading defense tower: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrading defense tower"));
		}
	}
	g_upgradingDefenseTowerMap = upgradingDefenseTowerMap;
	handles.push(DataSession::createServlet(UPGRADING_DEFENSE_TOWER_FILE, serializeCsv(csv, "tower_level")));
	handles.push(upgradingDefenseTowerMap);
}

namespace Data {
	boost::shared_ptr<const CastleBuildingBase> CastleBuildingBase::get(unsigned baseIndex){
		PROFILE_ME;

		const auto buildingBaseMap = g_buildingBaseMap.lock();
		if(!buildingBaseMap){
			LOG_EMPERY_CENTER_WARNING("CastleBuildingBaseMap has not been loaded.");
			return { };
		}

		const auto it = buildingBaseMap->find<0>(baseIndex);
		if(it == buildingBaseMap->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("CastleBuildingBase not found: baseIndex = ", baseIndex);
			return { };
		}
		return boost::shared_ptr<const CastleBuildingBase>(buildingBaseMap, &*it);
	}
	boost::shared_ptr<const CastleBuildingBase> CastleBuildingBase::require(unsigned baseIndex){
		PROFILE_ME;

		auto ret = get(baseIndex);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleBuildingBase not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleBuilding> CastleBuilding::get(BuildingId buildingId){
		PROFILE_ME;

		const auto buildingMap = g_buildingMap.lock();
		if(!buildingMap){
			LOG_EMPERY_CENTER_WARNING("CastleBuildingMap has not been loaded.");
			return { };
		}

		const auto it = buildingMap->find<0>(buildingId);
		if(it == buildingMap->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("CastleBuilding not found: buildingId = ", buildingId);
			return { };
		}
		return boost::shared_ptr<const CastleBuilding>(buildingMap, &*it);
	}
	boost::shared_ptr<const CastleBuilding> CastleBuilding::require(BuildingId buildingId){
		PROFILE_ME;

		auto ret = get(buildingId);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleBuilding base not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradingPrimary> CastleUpgradingPrimary::get(unsigned level){
		PROFILE_ME;

		const auto upgradingPrimaryMap = g_upgradingPrimaryMap.lock();
		if(!upgradingPrimaryMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradingPrimaryMap has not been loaded.");
			return { };
		}

		const auto it = upgradingPrimaryMap->find<0>(level);
		if(it == upgradingPrimaryMap->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradingPrimary not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradingPrimary>(upgradingPrimaryMap, &*it);
	}
	boost::shared_ptr<const CastleUpgradingPrimary> CastleUpgradingPrimary::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradingPrimary not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradingBarracks> CastleUpgradingBarracks::get(unsigned level){
		PROFILE_ME;

		const auto upgradingBarracksMap = g_upgradingBarracksMap.lock();
		if(!upgradingBarracksMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradingBarracksMap has not been loaded.");
			return { };
		}

		const auto it = upgradingBarracksMap->find<0>(level);
		if(it == upgradingBarracksMap->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradingBarracks not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradingBarracks>(upgradingBarracksMap, &*it);
	}
	boost::shared_ptr<const CastleUpgradingBarracks> CastleUpgradingBarracks::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradingBarracks not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradingAcademy> CastleUpgradingAcademy::get(unsigned level){
		PROFILE_ME;

		const auto upgradingAcademyMap = g_upgradingAcademyMap.lock();
		if(!upgradingAcademyMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradingAcademyMap has not been loaded.");
			return { };
		}

		const auto it = upgradingAcademyMap->find<0>(level);
		if(it == upgradingAcademyMap->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradingAcademy not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradingAcademy>(upgradingAcademyMap, &*it);
	}
	boost::shared_ptr<const CastleUpgradingAcademy> CastleUpgradingAcademy::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradingAcademy not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradingCivilian> CastleUpgradingCivilian::get(unsigned level){
		PROFILE_ME;

		const auto upgradingCivilianMap = g_upgradingCivilianMap.lock();
		if(!upgradingCivilianMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradingCivilianMap has not been loaded.");
			return { };
		}

		const auto it = upgradingCivilianMap->find<0>(level);
		if(it == upgradingCivilianMap->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradingCivilian not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradingCivilian>(upgradingCivilianMap, &*it);
	}
	boost::shared_ptr<const CastleUpgradingCivilian> CastleUpgradingCivilian::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradingCivilian not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradingWarehouse> CastleUpgradingWarehouse::get(unsigned level){
		PROFILE_ME;

		const auto upgradingWarehouseMap = g_upgradingWarehouseMap.lock();
		if(!upgradingWarehouseMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradingWarehouseMap has not been loaded.");
			return { };
		}

		const auto it = upgradingWarehouseMap->find<0>(level);
		if(it == upgradingWarehouseMap->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradingWarehouse not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradingWarehouse>(upgradingWarehouseMap, &*it);
	}
	boost::shared_ptr<const CastleUpgradingWarehouse> CastleUpgradingWarehouse::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradingWarehouse not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradingCitadelWall> CastleUpgradingCitadelWall::get(unsigned level){
		PROFILE_ME;

		const auto upgradingCitadelWallMap = g_upgradingCitadelWallMap.lock();
		if(!upgradingCitadelWallMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradingCitadelWallMap has not been loaded.");
			return { };
		}

		const auto it = upgradingCitadelWallMap->find<0>(level);
		if(it == upgradingCitadelWallMap->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradingCitadelWall not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradingCitadelWall>(upgradingCitadelWallMap, &*it);
	}
	boost::shared_ptr<const CastleUpgradingCitadelWall> CastleUpgradingCitadelWall::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradingCitadelWall not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradingDefenseTower> CastleUpgradingDefenseTower::get(unsigned level){
		PROFILE_ME;

		const auto upgradingDefenseTowerMap = g_upgradingDefenseTowerMap.lock();
		if(!upgradingDefenseTowerMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradingDefenseTowerMap has not been loaded.");
			return { };
		}

		const auto it = upgradingDefenseTowerMap->find<0>(level);
		if(it == upgradingDefenseTowerMap->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradingDefenseTower not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradingDefenseTower>(upgradingDefenseTowerMap, &*it);
	}
	boost::shared_ptr<const CastleUpgradingDefenseTower> CastleUpgradingDefenseTower::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradingDefenseTower not found"));
		}
		return ret;
	}
}

}
