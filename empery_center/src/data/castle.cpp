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

	using CastleUpgradePrimaryMap = boost::container::flat_map<unsigned, Data::CastleUpgradePrimary>;
	boost::weak_ptr<const CastleUpgradePrimaryMap> g_upgradePrimaryMap;
	const char UPGRADE_PRIMARY_FILE[] = "City_Castel";

	using CastleUpgradeBarracksMap = boost::container::flat_map<unsigned, Data::CastleUpgradeBarracks>;
	boost::weak_ptr<const CastleUpgradeBarracksMap> g_upgradeBarracksMap;
	const char UPGRADE_BARRACKS_FILE[] = "City_Camp";

	using CastleUpgradeAcademyMap = boost::container::flat_map<unsigned, Data::CastleUpgradeAcademy>;
	boost::weak_ptr<const CastleUpgradeAcademyMap> g_upgradeAcademyMap;
	const char UPGRADE_ACADEMY_FILE[] = "City_College";

	using CastleUpgradeCivilianMap = boost::container::flat_map<unsigned, Data::CastleUpgradeCivilian>;
	boost::weak_ptr<const CastleUpgradeCivilianMap> g_upgradeCivilianMap;
	const char UPGRADE_CIVILIAN_FILE[] = "City_House";

	using CastleUpgradeWarehouseMap = boost::container::flat_map<unsigned, Data::CastleUpgradeWarehouse>;
	boost::weak_ptr<const CastleUpgradeWarehouseMap> g_upgradeWarehouseMap;
	const char UPGRADE_WAREHOUSE_FILE[] = "City_Storage";

	using CastleUpgradeCitadelWallMap = boost::container::flat_map<unsigned, Data::CastleUpgradeCitadelWall>;
	boost::weak_ptr<const CastleUpgradeCitadelWallMap> g_upgradeCitadelWallMap;
	const char UPGRADE_CITADEL_WALL_FILE[] = "City_Wall";

	using CastleUpgradeDefenseTowerMap = boost::container::flat_map<unsigned, Data::CastleUpgradeDefenseTower>;
	boost::weak_ptr<const CastleUpgradeDefenseTowerMap> g_upgradeDefenseTowerMap;
	const char UPGRADE_DEFENSE_TOWER_FILE[] = "City_Tower";

	template<typename ElementT>
	void readUpgradeElement(ElementT &elem, const Poseidon::CsvParser &csv){
		boost::uint64_t minutes;
		csv.get(minutes, "levelup_time");
		elem.upgradeDuration = checkedMul<boost::uint64_t>(minutes, 1000);//XXX

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

		csv.get(elem.destructDuration, "demolition");
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

		if(!buildingMap->insert(std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate building: buildingId = ", elem.buildingId);
			DEBUG_THROW(Exception, sslit("Duplicate building"));
		}
	}
	g_buildingMap = buildingMap;
	handles.push(DataSession::createServlet(BUILDING_FILE, serializeCsv(csv, "id")));
	handles.push(buildingMap);

	const auto upgradePrimaryMap = boost::make_shared<CastleUpgradePrimaryMap>();
	path = dataDirectory + "/" + UPGRADE_PRIMARY_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrade primary: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradePrimary elem = { };

		csv.get(elem.level, "castel_level");
		readUpgradeElement(elem, csv);

		if(!upgradePrimaryMap->emplace(elem.level, std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrade primary: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrade primary"));
		}
	}
	g_upgradePrimaryMap = upgradePrimaryMap;
	handles.push(DataSession::createServlet(UPGRADE_PRIMARY_FILE, serializeCsv(csv, "castel_level")));
	handles.push(upgradePrimaryMap);

	const auto upgradeBarracksMap = boost::make_shared<CastleUpgradeBarracksMap>();
	path = dataDirectory + "/" + UPGRADE_BARRACKS_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrade barracks: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradeBarracks elem = { };

		csv.get(elem.level, "camp_level");
		readUpgradeElement(elem, csv);

		//

		if(!upgradeBarracksMap->emplace(elem.level, std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrade barracks: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrade barracks"));
		}
	}
	g_upgradeBarracksMap = upgradeBarracksMap;
	handles.push(DataSession::createServlet(UPGRADE_BARRACKS_FILE, serializeCsv(csv, "camp_level")));
	handles.push(upgradeBarracksMap);

	const auto upgradeAcademyMap = boost::make_shared<CastleUpgradeAcademyMap>();
	path = dataDirectory + "/" + UPGRADE_ACADEMY_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrade academy: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradeAcademy elem = { };

		csv.get(elem.level, "college_level");
		readUpgradeElement(elem, csv);

		csv.get(elem.techLevel, "tech_level");

		if(!upgradeAcademyMap->emplace(elem.level, std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrade academy: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrade academy"));
		}
	}
	g_upgradeAcademyMap = upgradeAcademyMap;
	handles.push(DataSession::createServlet(UPGRADE_ACADEMY_FILE, serializeCsv(csv, "college_level")));
	handles.push(upgradeAcademyMap);

	const auto upgradeCivilianMap = boost::make_shared<CastleUpgradeCivilianMap>();
	path = dataDirectory + "/" + UPGRADE_CIVILIAN_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrade civilian: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradeCivilian elem = { };

		csv.get(elem.level, "house_level");
		readUpgradeElement(elem, csv);

		csv.get(elem.maxPopulation, "population_max");

		if(!upgradeCivilianMap->emplace(elem.level, std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrade civilian: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrade civilian"));
		}
	}
	g_upgradeCivilianMap = upgradeCivilianMap;
	handles.push(DataSession::createServlet(UPGRADE_CIVILIAN_FILE, serializeCsv(csv, "house_level")));
	handles.push(upgradeCivilianMap);

	const auto upgradeWarehouseMap = boost::make_shared<CastleUpgradeWarehouseMap>();
	path = dataDirectory + "/" + UPGRADE_WAREHOUSE_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrade warehouse: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradeWarehouse elem = { };

		csv.get(elem.level, "storage_level");
		readUpgradeElement(elem, csv);

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

		if(!upgradeWarehouseMap->emplace(elem.level, std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrade warehouse: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrade warehouse"));
		}
	}
	g_upgradeWarehouseMap = upgradeWarehouseMap;
	handles.push(DataSession::createServlet(UPGRADE_WAREHOUSE_FILE, serializeCsv(csv, "storage_level")));
	handles.push(upgradeWarehouseMap);

	const auto upgradeCitadelWallMap = boost::make_shared<CastleUpgradeCitadelWallMap>();
	path = dataDirectory + "/" + UPGRADE_CITADEL_WALL_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrade citadel wall: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradeCitadelWall elem = { };

		csv.get(elem.level, "wall_level");
		readUpgradeElement(elem, csv);

		csv.get(elem.strength, "troops");
		csv.get(elem.armor, "defence");

		if(!upgradeCitadelWallMap->emplace(elem.level, std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrade citadel wall: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrade citadel wall"));
		}
	}
	g_upgradeCitadelWallMap = upgradeCitadelWallMap;
	handles.push(DataSession::createServlet(UPGRADE_CITADEL_WALL_FILE, serializeCsv(csv, "wall_level")));
	handles.push(upgradeCitadelWallMap);

	const auto upgradeDefenseTowerMap = boost::make_shared<CastleUpgradeDefenseTowerMap>();
	path = dataDirectory + "/" + UPGRADE_DEFENSE_TOWER_FILE + ".csv";
	LOG_EMPERY_CENTER_INFO("Loading castle upgrade defense tower: path = ", path);
	csv.load(path.c_str());
	while(csv.fetchRow()){
		Data::CastleUpgradeDefenseTower elem = { };

		csv.get(elem.level, "tower_level");
		readUpgradeElement(elem, csv);

		csv.get(elem.firepower, "atk");

		if(!upgradeDefenseTowerMap->emplace(elem.level, std::move(elem)).second){
			LOG_EMPERY_CENTER_ERROR("Duplicate upgrade defense tower: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate upgrade defense tower"));
		}
	}
	g_upgradeDefenseTowerMap = upgradeDefenseTowerMap;
	handles.push(DataSession::createServlet(UPGRADE_DEFENSE_TOWER_FILE, serializeCsv(csv, "tower_level")));
	handles.push(upgradeDefenseTowerMap);
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

	boost::shared_ptr<const CastleUpgradeAbstract> CastleUpgradeAbstract::get(CastleBuilding::Type type, unsigned level){
		PROFILE_ME;

		switch(type){
		case CastleBuilding::T_PRIMARY:
			return CastleUpgradePrimary::get(level);
		case CastleBuilding::T_BARRACKS:
			return CastleUpgradeBarracks::get(level);
		case CastleBuilding::T_ACADEMY:
			return CastleUpgradeAcademy::get(level);
		case CastleBuilding::T_CIVILIAN:
			return CastleUpgradeCivilian::get(level);
//		case CastleBuilding::T_WATCHTOWER:
//			return CastleUpgradeWatchtower::get(level);
		case CastleBuilding::T_WAREHOUSE:
			return CastleUpgradeWarehouse::get(level);
		case CastleBuilding::T_CITADEL_WALL:
			return CastleUpgradeCitadelWall::get(level);
		case CastleBuilding::T_DEFENSE_TOWER:
			return CastleUpgradeDefenseTower::get(level);
		default:
			LOG_EMPERY_CENTER_WARNING("Unknown building type: type = ", static_cast<unsigned>(type));
			return { };
		}
	}
	boost::shared_ptr<const CastleUpgradeAbstract> CastleUpgradeAbstract::require(CastleBuilding::Type type, unsigned level){
		PROFILE_ME;

		auto ret = get(type, level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradeAbstract not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradePrimary> CastleUpgradePrimary::get(unsigned level){
		PROFILE_ME;

		const auto upgradePrimaryMap = g_upgradePrimaryMap.lock();
		if(!upgradePrimaryMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradePrimaryMap has not been loaded.");
			return { };
		}

		const auto it = upgradePrimaryMap->find(level);
		if(it == upgradePrimaryMap->end()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradePrimary not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradePrimary>(upgradePrimaryMap, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradePrimary> CastleUpgradePrimary::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradePrimary not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeBarracks> CastleUpgradeBarracks::get(unsigned level){
		PROFILE_ME;

		const auto upgradeBarracksMap = g_upgradeBarracksMap.lock();
		if(!upgradeBarracksMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeBarracksMap has not been loaded.");
			return { };
		}

		const auto it = upgradeBarracksMap->find(level);
		if(it == upgradeBarracksMap->end()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeBarracks not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeBarracks>(upgradeBarracksMap, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeBarracks> CastleUpgradeBarracks::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradeBarracks not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeAcademy> CastleUpgradeAcademy::get(unsigned level){
		PROFILE_ME;

		const auto upgradeAcademyMap = g_upgradeAcademyMap.lock();
		if(!upgradeAcademyMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeAcademyMap has not been loaded.");
			return { };
		}

		const auto it = upgradeAcademyMap->find(level);
		if(it == upgradeAcademyMap->end()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeAcademy not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeAcademy>(upgradeAcademyMap, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeAcademy> CastleUpgradeAcademy::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradeAcademy not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeCivilian> CastleUpgradeCivilian::get(unsigned level){
		PROFILE_ME;

		const auto upgradeCivilianMap = g_upgradeCivilianMap.lock();
		if(!upgradeCivilianMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeCivilianMap has not been loaded.");
			return { };
		}

		const auto it = upgradeCivilianMap->find(level);
		if(it == upgradeCivilianMap->end()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeCivilian not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeCivilian>(upgradeCivilianMap, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeCivilian> CastleUpgradeCivilian::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradeCivilian not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeWarehouse> CastleUpgradeWarehouse::get(unsigned level){
		PROFILE_ME;

		const auto upgradeWarehouseMap = g_upgradeWarehouseMap.lock();
		if(!upgradeWarehouseMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeWarehouseMap has not been loaded.");
			return { };
		}

		const auto it = upgradeWarehouseMap->find(level);
		if(it == upgradeWarehouseMap->end()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeWarehouse not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeWarehouse>(upgradeWarehouseMap, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeWarehouse> CastleUpgradeWarehouse::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradeWarehouse not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeCitadelWall> CastleUpgradeCitadelWall::get(unsigned level){
		PROFILE_ME;

		const auto upgradeCitadelWallMap = g_upgradeCitadelWallMap.lock();
		if(!upgradeCitadelWallMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeCitadelWallMap has not been loaded.");
			return { };
		}

		const auto it = upgradeCitadelWallMap->find(level);
		if(it == upgradeCitadelWallMap->end()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeCitadelWall not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeCitadelWall>(upgradeCitadelWallMap, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeCitadelWall> CastleUpgradeCitadelWall::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradeCitadelWall not found"));
		}
		return ret;
	}

	boost::shared_ptr<const CastleUpgradeDefenseTower> CastleUpgradeDefenseTower::get(unsigned level){
		PROFILE_ME;

		const auto upgradeDefenseTowerMap = g_upgradeDefenseTowerMap.lock();
		if(!upgradeDefenseTowerMap){
			LOG_EMPERY_CENTER_WARNING("CastleUpgradeDefenseTowerMap has not been loaded.");
			return { };
		}

		const auto it = upgradeDefenseTowerMap->find(level);
		if(it == upgradeDefenseTowerMap->end()){
			LOG_EMPERY_CENTER_DEBUG("CastleUpgradeDefenseTower not found: level = ", level);
			return { };
		}
		return boost::shared_ptr<const CastleUpgradeDefenseTower>(upgradeDefenseTowerMap, &(it->second));
	}
	boost::shared_ptr<const CastleUpgradeDefenseTower> CastleUpgradeDefenseTower::require(unsigned level){
		PROFILE_ME;

		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("CastleUpgradeDefenseTower not found"));
		}
		return ret;
	}
}

}
