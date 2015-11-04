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
		msg.missionTimeRemaining = saturatedSub(it->missionTimeEnd, Poseidon::getUtcTime());
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
		return { Msg::CERR_ANOTHER_BUILDING_THERE, castToStr(req.baseIndex) };
	}

	const auto buildingBaseData = Data::CastleBuildingBase::get(req.baseIndex);
	if(!buildingBaseData){
		return { Msg::CERR_NO_SUCH_CASTLE_BASE, castToStr(req.baseIndex) };
	}
	if(buildingBaseData->buildingsAllowed.find(BuildingId(req.buildingId)) == buildingBaseData->buildingsAllowed.end()){
		return { Msg::CERR_BUILDING_NOT_ALLOWED, castToStr(req.buildingId) };
	}

	castle->createMission(req.baseIndex, Castle::MIS_CONSTRUCT, BuildingId(req.buildingId),
		0, 0, 0); // TODO

	return { };
}

PLAYER_SERVLET(Msg::CS_CastleCancelMission, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));

	const auto info = castle->getBuilding(req.baseIndex);
	if(info.mission == Castle::MIS_NONE){
		return { Msg::CERR_NO_CASTLE_MISSION, castToStr(req.baseIndex) };
	}

	castle->cancelMission(req.baseIndex);

	return { };
}

PLAYER_SERVLET(Msg::CS_CastleUpgradeBuilding, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));

	const auto info = castle->getBuilding(req.baseIndex);
	if(info.buildingId == BuildingId()){
		return { Msg::CERR_NO_BUILDING_THERE, castToStr(req.baseIndex) };
	}
	if(info.mission != Castle::MIS_NONE){
		return { Msg::CERR_CASTLE_MISSION_CONFLICT, castToStr(req.baseIndex) };
	}

	castle->createMission(req.baseIndex, Castle::MIS_UPGRADE, BuildingId(),
		0, 0, 0);// TODO

	return { };
}

PLAYER_SERVLET(Msg::CS_CastleDestroyBuilding, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));

	const auto info = castle->getBuilding(req.baseIndex);
	if(info.buildingId == BuildingId()){
		return { Msg::CERR_NO_BUILDING_THERE, castToStr(req.baseIndex) };
	}
	if(info.mission != Castle::MIS_NONE){
		return { Msg::CERR_CASTLE_MISSION_CONFLICT, castToStr(req.baseIndex) };
	}

	castle->createMission(req.baseIndex, Castle::MIS_DESTRUCT, BuildingId(),
		0, 0, 0);// TODO

	return { };
}

PLAYER_SERVLET(Msg::CS_CastleCompleteImmediately, accountUuid, session, req){
	const auto castle = requireCastle(accountUuid, MapObjectUuid(req.mapObjectUuid));

	const auto info = castle->getBuilding(req.baseIndex);
	if(info.mission == Castle::MIS_NONE){
		return { Msg::CERR_NO_CASTLE_MISSION, castToStr(req.baseIndex) };
	}

	// TODO
	castle->completeMission(req.baseIndex);

	return { };
}

}
