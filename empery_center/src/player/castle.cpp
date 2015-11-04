#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_castle.hpp"
#include "../msg/sc_castle.hpp"
#include "../msg/cerr_castle.hpp"
#include "../singletons/map_object_map.hpp"
#include "../castle.hpp"
#include "../checked_arithmetic.hpp"
#include "../data/castle.hpp"

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
		castle->pumpStatus();
		return castle;
	}
}

PLAYER_SERVLET(Msg::CS_CastleQueryInfo, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));

	const auto utcNow = Poseidon::getUtcTime();

	std::vector<Castle::BuildingInfo> buildings;
	castle->getAllBuildings(buildings);
	for(auto it = buildings.begin(); it != buildings.end(); ++it){
		Msg::SC_CastleBuildingBase msg;
		msg.mapObjectUuid        = castle->getMapObjectUuid().str();
		msg.baseIndex            = it->baseIndex;
		msg.buildingId           = it->buildingId.get();
		msg.buildingLevel        = it->buildingLevel;
		msg.mission              = it->mission;
		msg.missionParam1        = it->missionParam1;
		msg.missionParam2        = it->missionParam2;
		msg.missionTimeBegin     = it->missionTimeBegin;
		msg.missionTimeRemaining = saturatedSub(it->missionTimeEnd, utcNow);
		session->send(msg);
	}

	std::vector<Castle::ResourceInfo> resources;
	castle->getAllResources(resources);
	for(auto it = resources.begin(); it != resources.end(); ++it){
		Msg::SC_CastleResource msg;
		msg.mapObjectUuid   = castle->getMapObjectUuid().str();
		msg.resourceId      = it->resourceId.get();
		msg.count           = it->count;
		session->send(msg);
	}

	return { };
}

PLAYER_SERVLET(Msg::CS_CastleCreateBuilding, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));

	const auto info = castle->getBuilding(req.baseIndex);
	if(info.mission != Castle::MIS_NONE){
		return { Msg::CERR_ANOTHER_BUILDING_THERE, strCast(req.baseIndex) };
	}

	const auto buildingBaseData = Data::CastleBuildingBase::get(req.baseIndex);
	if(!buildingBaseData){
		return { Msg::CERR_NO_SUCH_CASTLE_BASE, strCast(req.baseIndex) };
	}
	if(buildingBaseData->buildingsAllowed.find(BuildingId(req.buildingId)) == buildingBaseData->buildingsAllowed.end()){
		return { Msg::CERR_BUILDING_NOT_ALLOWED, strCast(req.buildingId) };
	}

	const auto buildingData = Data::CastleBuilding::require(BuildingId(req.buildingId));
	const auto upgradeData = Data::CastleUpgradeAbstract::require(buildingData->type, 1); // 建造建筑就是升到 1 级。
	for(auto it = upgradeData->prerequisite.begin(); it != upgradeData->prerequisite.end(); ++it){
		const auto otherInfo = castle->getBuildingById(it->first);
		if(otherInfo.buildingLevel < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: buildingId = ", it->first,
				", levelRequired = ", it->second, ", buildingLevel = ", info.buildingLevel);
			return { Msg::CERR_PREREQUISITE_NOT_MET, strCast(it->first) };
		}
	}
	std::vector<Castle::ResourceTransactionElement> transaction;
	for(auto it = upgradeData->upgradeCost.begin(); it != upgradeData->upgradeCost.end(); ++it){
		transaction.emplace_back(Castle::ResourceTransactionElement::OP_REMOVE, it->first, it->second);
	}
	const auto insuffResourceId = castle->commitResourceTransactionNoThrow(transaction.data(), transaction.size(),
		[&]{ castle->createMission(req.baseIndex, Castle::MIS_CONSTRUCT, BuildingId(req.buildingId)); });
	if(insuffResourceId){
		return { Msg::CERR_CASTLE_NO_ENOUGH_RESOURCES, strCast(insuffResourceId) };
	}

	return { };
}

PLAYER_SERVLET(Msg::CS_CastleCancelMission, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));

	const auto info = castle->getBuilding(req.baseIndex);
	if(info.mission == Castle::MIS_NONE){
		return { Msg::CERR_NO_CASTLE_MISSION, strCast(req.baseIndex) };
	}

	castle->cancelMission(req.baseIndex);

	return { };
}

PLAYER_SERVLET(Msg::CS_CastleUpgradeBuilding, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));

	const auto info = castle->getBuilding(req.baseIndex);
	if(info.buildingId == BuildingId()){
		return { Msg::CERR_NO_BUILDING_THERE, strCast(req.baseIndex) };
	}
	if(info.mission != Castle::MIS_NONE){
		return { Msg::CERR_CASTLE_MISSION_CONFLICT, strCast(req.baseIndex) };
	}

	const auto buildingData = Data::CastleBuilding::require(info.buildingId);
	const auto upgradeData = Data::CastleUpgradeAbstract::get(buildingData->type, info.buildingLevel + 1);
	if(!upgradeData){
		return { Msg::CERR_CASTLE_UPGRADE_MAX, strCast(info.buildingId) };
	}
	for(auto it = upgradeData->prerequisite.begin(); it != upgradeData->prerequisite.end(); ++it){
		const auto otherInfo = castle->getBuildingById(it->first);
		if(otherInfo.buildingLevel < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: buildingId = ", it->first,
				", levelRequired = ", it->second, ", buildingLevel = ", info.buildingLevel);
			return { Msg::CERR_PREREQUISITE_NOT_MET, strCast(it->first) };
		}
	}
	std::vector<Castle::ResourceTransactionElement> transaction;
	for(auto it = upgradeData->upgradeCost.begin(); it != upgradeData->upgradeCost.end(); ++it){
		transaction.emplace_back(Castle::ResourceTransactionElement::OP_REMOVE, it->first, it->second);
	}
	const auto insuffResourceId = castle->commitResourceTransactionNoThrow(transaction.data(), transaction.size(),
		[&]{ castle->createMission(req.baseIndex, Castle::MIS_UPGRADE, { }); });
	if(insuffResourceId){
		return { Msg::CERR_CASTLE_NO_ENOUGH_RESOURCES, strCast(insuffResourceId) };
	}

	return { };
}

PLAYER_SERVLET(Msg::CS_CastleDestroyBuilding, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));

	const auto info = castle->getBuilding(req.baseIndex);
	if(info.buildingId == BuildingId()){
		return { Msg::CERR_NO_BUILDING_THERE, strCast(req.baseIndex) };
	}
	if(info.mission != Castle::MIS_NONE){
		return { Msg::CERR_CASTLE_MISSION_CONFLICT, strCast(req.baseIndex) };
	}

	const auto buildingData = Data::CastleBuilding::require(info.buildingId);
	const auto upgradeData = Data::CastleUpgradeAbstract::require(buildingData->type, info.buildingLevel);
	if(upgradeData->destructDuration == 0){
		return { Msg::CERR_BUILDING_NOT_DESTRUCTIBLE, strCast(info.buildingId) };
	}
	std::vector<Castle::ResourceTransactionElement> transaction;
	if((info.mission == Castle::MIS_CONSTRUCT) || (info.mission == Castle::MIS_UPGRADE)){
		const auto refundRatio = getConfig<double>("castle_building_cancellation_refund_ratio", 0.5);
		for(auto it = upgradeData->upgradeCost.begin(); it != upgradeData->upgradeCost.end(); ++it){
			transaction.emplace_back(Castle::ResourceTransactionElement::OP_ADD, it->first,
				static_cast<boost::uint64_t>(std::floor(it->second * refundRatio)));
		}
	}
	castle->commitResourceTransaction(transaction.data(), transaction.size(),
		[&]{ castle->cancelMission(req.baseIndex); });

	return { };
}

PLAYER_SERVLET(Msg::CS_CastleCompleteImmediately, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));

	const auto info = castle->getBuilding(req.baseIndex);
	if(info.mission == Castle::MIS_NONE){
		return { Msg::CERR_NO_CASTLE_MISSION, strCast(req.baseIndex) };
	}

	// TODO 计算消耗。
	castle->speedUpMission(req.baseIndex, UINT64_MAX);

	return { };
}

PLAYER_SERVLET(Msg::CS_CastleQueryIndividualBaseInfo, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));

	const auto utcNow = Poseidon::getUtcTime();
	const auto info = castle->getBuilding(req.baseIndex);

	Msg::SC_CastleBuildingBase msg;
	msg.mapObjectUuid        = castle->getMapObjectUuid().str();
	msg.baseIndex            = info.baseIndex;
	msg.buildingId           = info.buildingId.get();
	msg.buildingLevel        = info.buildingLevel;
	msg.mission              = info.mission;
	msg.missionParam1        = info.missionParam1;
	msg.missionParam2        = info.missionParam2;
	msg.missionTimeBegin     = info.missionTimeBegin;
	msg.missionTimeRemaining = saturatedSub(info.missionTimeEnd, utcNow);
	session->send(msg);

	return { };
}

}
