#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_castle.hpp"
#include "../msg/cerr_castle.hpp"
#include "../singletons/map_object_map.hpp"
#include "../castle.hpp"
#include "../checked_arithmetic.hpp"
#include "../data/castle.hpp"
#include "../reason_ids.hpp"

namespace EmperyCenter {

namespace {
	inline boost::shared_ptr<Castle> requireCastle(const AccountUuid &accountUuid, const MapObjectUuid &mapObjectUuid){
		PROFILE_ME;

		const auto castle = boost::dynamic_pointer_cast<Castle>(MapObjectMap::get(mapObjectUuid));
		if(!castle){
			DEBUG_THROW(Poseidon::Cbpp::Exception, Msg::CERR_NO_SUCH_CASTLE, SharedNts(mapObjectUuid.str()));
		}
		if(castle->getOwnerUuid() != accountUuid){
			DEBUG_THROW(Poseidon::Cbpp::Exception, Msg::CERR_NOT_CASTLE_OWNER, SharedNts(castle->getOwnerUuid().str()));
		}
		return castle;
	}
}

PLAYER_SERVLET(Msg::CS_CastleQueryInfo, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));
	castle->pumpStatus(true);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCreateBuilding, accountUuid, session, req){
	const auto buildingBaseId = BuildingBaseId(req.buildingBaseId);
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));
	castle->pumpBuildingStatus(buildingBaseId);

	const auto info = castle->getBuilding(buildingBaseId);
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::CERR_ANOTHER_BUILDING_THERE) <<buildingBaseId;
	}

	const auto buildingId = BuildingId(req.buildingId);
	const auto buildingData = Data::CastleBuilding::get(buildingId);
	if(!buildingData){
		return Response(Msg::CERR_NO_SUCH_BUILDING) <<buildingId;
	}
	const auto count = castle->countBuildingsById(buildingId);
	if(count >= buildingData->buildLimit){
		return Response(Msg::CERR_BUILD_LIMIT_EXCEEDED) <<buildingId;
	}

	const auto buildingBaseData = Data::CastleBuildingBase::get(buildingBaseId);
	if(!buildingBaseData){
		return Response(Msg::CERR_NO_SUCH_CASTLE_BASE) <<buildingBaseId;
	}
	if(buildingBaseData->buildingsAllowed.find(buildingData->buildingId) == buildingBaseData->buildingsAllowed.end()){
		return Response(Msg::CERR_BUILDING_NOT_ALLOWED) <<buildingId;
	}

	const auto upgradeData = Data::CastleUpgradeAbstract::require(buildingData->type, 1);
	for(auto it = upgradeData->prerequisite.begin(); it != upgradeData->prerequisite.end(); ++it){
		std::vector<Castle::BuildingInfo> prerequisiteBuildings;
		castle->getBuildingsById(prerequisiteBuildings, it->first);
		unsigned maxLevel = 0;
		for(auto prereqIt = prerequisiteBuildings.begin(); prereqIt != prerequisiteBuildings.end(); ++prereqIt){
			if(maxLevel < prereqIt->buildingLevel){
				maxLevel = prereqIt->buildingLevel;
			}
		}
		if(maxLevel < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: buildingId = ", it->first,
				", levelRequired = ", it->second, ", maxLevel = ", maxLevel);
			return Response(Msg::CERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	std::vector<Castle::ResourceTransactionElement> transaction;
	for(auto it = upgradeData->upgradeCost.begin(); it != upgradeData->upgradeCost.end(); ++it){
		transaction.emplace_back(Castle::ResourceTransactionElement::OP_REMOVE, it->first, it->second,
			ReasonIds::ID_UPGRADE_BUILDING, buildingData->buildingId.get(), upgradeData->buildingLevel, 0);
	}
	const auto insuffResourceId = castle->commitResourceTransactionNoThrow(transaction.data(), transaction.size(),
		[&]{ castle->createBuildingMission(buildingBaseId, Castle::MIS_CONSTRUCT, buildingData->buildingId); });
	if(insuffResourceId){
		return Response(Msg::CERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuffResourceId;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCancelBuildingMission, accountUuid, session, req){
	const auto buildingBaseId = BuildingBaseId(req.buildingBaseId);
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));
	castle->pumpBuildingStatus(buildingBaseId);

	const auto info = castle->getBuilding(buildingBaseId);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::CERR_NO_BUILDING_MISSION) <<buildingBaseId;
	}

	const auto buildingData = Data::CastleBuilding::require(info.buildingId);
	const auto upgradeData = Data::CastleUpgradeAbstract::require(buildingData->type, info.buildingLevel + 1);
	std::vector<Castle::ResourceTransactionElement> transaction;
	if((info.mission == Castle::MIS_CONSTRUCT) || (info.mission == Castle::MIS_UPGRADE)){
		const auto refundRatio = getConfig<double>("castle_cancellation_refund_ratio", 0.5);
		for(auto it = upgradeData->upgradeCost.begin(); it != upgradeData->upgradeCost.end(); ++it){
			transaction.emplace_back(Castle::ResourceTransactionElement::OP_ADD,
				it->first, static_cast<boost::uint64_t>(std::floor(it->second * refundRatio)),
				ReasonIds::ID_CANCEL_UPGRADE_BUILDING, info.buildingId.get(), info.buildingLevel, 0);
		}
	}
	castle->commitResourceTransaction(transaction.data(), transaction.size(),
		[&]{ castle->cancelBuildingMission(buildingBaseId); });

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUpgradeBuilding, accountUuid, session, req){
	const auto buildingBaseId = BuildingBaseId(req.buildingBaseId);
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));
	castle->pumpBuildingStatus(buildingBaseId);

	const auto info = castle->getBuilding(buildingBaseId);
	if(info.buildingId == BuildingId()){
		return Response(Msg::CERR_NO_BUILDING_THERE) <<buildingBaseId;
	}
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::CERR_BUILDING_MISSION_CONFLICT) <<buildingBaseId;
	}

	const auto buildingData = Data::CastleBuilding::require(info.buildingId);
	const auto upgradeData = Data::CastleUpgradeAbstract::get(buildingData->type, info.buildingLevel + 1);
	if(!upgradeData){
		return Response(Msg::CERR_BUILDING_UPGRADE_MAX) <<info.buildingId;
	}
	for(auto it = upgradeData->prerequisite.begin(); it != upgradeData->prerequisite.end(); ++it){
		std::vector<Castle::BuildingInfo> prerequisiteBuildings;
		castle->getBuildingsById(prerequisiteBuildings, it->first);
		unsigned maxLevel = 0;
		for(auto prereqIt = prerequisiteBuildings.begin(); prereqIt != prerequisiteBuildings.end(); ++prereqIt){
			if(maxLevel < prereqIt->buildingLevel){
				maxLevel = prereqIt->buildingLevel;
			}
		}
		if(maxLevel < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: buildingId = ", it->first,
				", levelRequired = ", it->second, ", maxLevel = ", maxLevel);
			return Response(Msg::CERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	std::vector<Castle::ResourceTransactionElement> transaction;
	for(auto it = upgradeData->upgradeCost.begin(); it != upgradeData->upgradeCost.end(); ++it){
		transaction.emplace_back(Castle::ResourceTransactionElement::OP_REMOVE, it->first, it->second,
			ReasonIds::ID_UPGRADE_BUILDING, buildingData->buildingId.get(), upgradeData->buildingLevel, 0);
	}
	const auto insuffResourceId = castle->commitResourceTransactionNoThrow(transaction.data(), transaction.size(),
		[&]{ castle->createBuildingMission(buildingBaseId, Castle::MIS_UPGRADE, { }); });
	if(insuffResourceId){
		return Response(Msg::CERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuffResourceId;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleDestroyBuilding, accountUuid, session, req){
	const auto buildingBaseId = BuildingBaseId(req.buildingBaseId);
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));
	castle->pumpBuildingStatus(buildingBaseId);

	const auto info = castle->getBuilding(buildingBaseId);
	if(info.buildingId == BuildingId()){
		return Response(Msg::CERR_NO_BUILDING_THERE) <<buildingBaseId;
	}
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::CERR_BUILDING_MISSION_CONFLICT) <<buildingBaseId;
	}

	const auto buildingData = Data::CastleBuilding::require(info.buildingId);
	const auto upgradeData = Data::CastleUpgradeAbstract::require(buildingData->type, info.buildingLevel);
	if(upgradeData->destructDuration == 0){
		return Response(Msg::CERR_BUILDING_NOT_DESTRUCTIBLE) <<info.buildingId;
	}

	castle->createBuildingMission(buildingBaseId, Castle::MIS_DESTRUCT, BuildingId());

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCompleteBuildingImmediately, accountUuid, session, req){
	const auto buildingBaseId = BuildingBaseId(req.buildingBaseId);
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));
	castle->pumpBuildingStatus(buildingBaseId);

	const auto info = castle->getBuilding(buildingBaseId);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::CERR_NO_BUILDING_MISSION) <<buildingBaseId;
	}

	// TODO 计算消耗。
	castle->speedUpBuildingMission(buildingBaseId, UINT64_MAX);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryIndividualBuildingInfo, accountUuid, session, req){
	const auto buildingBaseId = BuildingBaseId(req.buildingBaseId);
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));
	castle->pumpBuildingStatus(buildingBaseId, true);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUpgradeTech, accountUuid, session, req){
	const auto techId = TechId(req.techId);
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));
	castle->pumpTechStatus(techId);

	const auto info = castle->getTech(techId);
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::CERR_TECH_MISSION_CONFLICT) <<techId;
	}

	const auto techData = Data::CastleTech::get(TechId(req.techId), info.techLevel + 1);
	if(!techData){
		if(info.techLevel == 0){
			return Response(Msg::CERR_NO_SUCH_TECH) <<techId;
		}
		return Response(Msg::CERR_TECH_UPGRADE_MAX) <<techId;
	}

	for(auto it = techData->prerequisite.begin(); it != techData->prerequisite.end(); ++it){
		std::vector<Castle::BuildingInfo> prerequisiteBuildings;
		castle->getBuildingsById(prerequisiteBuildings, it->first);
		unsigned maxLevel = 0;
		for(auto prereqIt = prerequisiteBuildings.begin(); prereqIt != prerequisiteBuildings.end(); ++prereqIt){
			if(maxLevel < prereqIt->buildingLevel){
				maxLevel = prereqIt->buildingLevel;
			}
		}
		if(maxLevel < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: techId = ", it->first,
				", levelRequired = ", it->second, ", maxLevel = ", maxLevel);
			return Response(Msg::CERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	for(auto it = techData->displayPrerequisite.begin(); it != techData->displayPrerequisite.end(); ++it){
		std::vector<Castle::BuildingInfo> prerequisiteBuildings;
		castle->getBuildingsById(prerequisiteBuildings, it->first);
		unsigned maxLevel = 0;
		for(auto prereqIt = prerequisiteBuildings.begin(); prereqIt != prerequisiteBuildings.end(); ++prereqIt){
			if(maxLevel < prereqIt->buildingLevel){
				maxLevel = prereqIt->buildingLevel;
			}
		}
		if(maxLevel < it->second){
			LOG_EMPERY_CENTER_DEBUG("Display prerequisite not met: techId = ", it->first,
				", levelRequired = ", it->second, ", maxLevel = ", maxLevel);
			return Response(Msg::CERR_DISPLAY_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	std::vector<Castle::ResourceTransactionElement> transaction;
	for(auto it = techData->upgradeCost.begin(); it != techData->upgradeCost.end(); ++it){
		transaction.emplace_back(Castle::ResourceTransactionElement::OP_REMOVE, it->first, it->second,
			ReasonIds::ID_UPGRADE_TECH, techData->techIdLevel.first.get(), techData->techIdLevel.second, 0);
	}
	const auto insuffResourceId = castle->commitResourceTransactionNoThrow(transaction.data(), transaction.size(),
		[&]{ castle->createTechMission(techId, Castle::MIS_UPGRADE); });
	if(insuffResourceId){
		return Response(Msg::CERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuffResourceId;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCancelTechMission, accountUuid, session, req){
	const auto techId = TechId(req.techId);
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));
	castle->pumpTechStatus(techId);

	const auto info = castle->getTech(techId);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::CERR_NO_TECH_MISSION) <<techId;
	}

	const auto techData = Data::CastleTech::require(info.techId, info.techLevel + 1);
	std::vector<Castle::ResourceTransactionElement> transaction;
	if((info.mission == Castle::MIS_CONSTRUCT) || (info.mission == Castle::MIS_UPGRADE)){
		const auto refundRatio = getConfig<double>("castle_cancellation_refund_ratio", 0.5);
		for(auto it = techData->upgradeCost.begin(); it != techData->upgradeCost.end(); ++it){
			transaction.emplace_back(Castle::ResourceTransactionElement::OP_ADD,
				it->first, static_cast<boost::uint64_t>(std::floor(it->second * refundRatio)),
				ReasonIds::ID_CANCEL_UPGRADE_TECH, info.techId.get(), info.techLevel, 0);
		}
	}
	castle->commitResourceTransaction(transaction.data(), transaction.size(),
		[&]{ castle->cancelTechMission(techId); });

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCompleteTechImmediately, accountUuid, session, req){
	const auto techId = TechId(req.techId);
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));
	castle->pumpTechStatus(techId);

	const auto info = castle->getTech(techId);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::CERR_NO_TECH_MISSION) <<techId;
	}

	// TODO 计算消耗。
	castle->speedUpTechMission(techId, UINT64_MAX);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryIndividualTechInfo, accountUuid, session, req){
	const auto techId = TechId(req.techId);
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));
	castle->pumpTechStatus(techId, true);

	return Response();
}

}
