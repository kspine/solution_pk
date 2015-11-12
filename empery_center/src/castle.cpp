#include "precompiled.hpp"
#include "castle.hpp"
#include "map_object.hpp"
#include "mysql/castle_building_base.hpp"
#include "mysql/castle_tech.hpp"
#include "mysql/castle_resource.hpp"
#include "msg/sc_castle.hpp"
#include "singletons/player_session_map.hpp"
#include "checked_arithmetic.hpp"
#include "player_session.hpp"
#include "data/castle.hpp"
#include "events/resource.hpp"

namespace EmperyCenter {

namespace {
	void createInitBuildings(const MapObjectUuid &mapObjectUuid,
		boost::container::flat_map<BuildingBaseId, boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings)
	{
		PROFILE_ME;

		std::vector<boost::shared_ptr<const Data::CastleBuildingBase>> initBuildings;
		Data::CastleBuildingBase::getInit(initBuildings);
		for(auto it = initBuildings.begin(); it != initBuildings.end(); ++it){
			const auto &buildingData = *it;
			const auto &buildingsAllowed = buildingData->buildingsAllowed;
			if(buildingsAllowed.empty()){
				continue;
			}
			const auto buildingBaseId = buildingData->buildingBaseId;
			const auto initLevel = buildingData->initLevel;
			const auto buildingIt = buildings.find(buildingBaseId);
			if(buildingIt == buildings.end()){
				const auto buildingId = *(buildingsAllowed.begin());
				LOG_EMPERY_CENTER_DEBUG("> Creating init building: mapObjectUuid = ", mapObjectUuid,
					", buildingBaseId = ", buildingBaseId, ", buildingId = ", buildingId, ", initLevel = ", initLevel);
				auto obj = boost::make_shared<MySql::Center_CastleBuildingBase>(
					mapObjectUuid.get(), buildingBaseId.get(), buildingId.get(), initLevel, Castle::MIS_NONE, 0, 0, 0, 0);
				obj->asyncSave(true);
				buildings.emplace(buildingBaseId, std::move(obj));
			} else {
				const auto &obj = buildingIt->second;
				const unsigned oldLevel = obj->get_buildingLevel();
				if(oldLevel < initLevel){
					LOG_EMPERY_CENTER_DEBUG("> Upgrading init building: mapObjectUuid = ", mapObjectUuid,
						", buildingBaseId = ", buildingBaseId, ", oldLevel = ", oldLevel, ", initLevel = ", initLevel);
					obj->set_buildingLevel(initLevel);
				}
			}
		}
	}

	void fillBuildingInfo(Castle::BuildingInfo &info, const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj){
		PROFILE_ME;

		info.buildingBaseId   = BuildingBaseId(obj->get_buildingBaseId());
		info.buildingId       = BuildingId(obj->get_buildingId());
		info.buildingLevel    = obj->get_buildingLevel();
		info.mission          = Castle::Mission(obj->get_mission());
		info.missionDuration    = obj->get_missionDuration();
		info.missionParam2    = obj->get_missionParam2();
		info.missionTimeBegin = obj->get_missionTimeBegin();
		info.missionTimeEnd   = obj->get_missionTimeEnd();
	}
	void fillTechInfo(Castle::TechInfo &info, const boost::shared_ptr<MySql::Center_CastleTech> &obj){
		PROFILE_ME;

		info.techId           = TechId(obj->get_techId());
		info.techLevel        = obj->get_techLevel();
		info.mission          = Castle::Mission(obj->get_mission());
		info.missionDuration    = obj->get_missionDuration();
		info.missionParam2    = obj->get_missionParam2();
		info.missionTimeBegin = obj->get_missionTimeBegin();
		info.missionTimeEnd   = obj->get_missionTimeEnd();
	}
	void fillResourceInfo(Castle::ResourceInfo &info, const boost::shared_ptr<MySql::Center_CastleResource> &obj){
		PROFILE_ME;

		info.resourceId       = ResourceId(obj->get_resourceId());
		info.count            = obj->get_count();
	}

	void synchronizeBuildingBaseWithClient(const MapObjectUuid &mapObjectUuid, const AccountUuid &ownerUuid,
		const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj)
	{
		PROFILE_ME;

		const auto session = PlayerSessionMap::get(ownerUuid);
		if(session){
			try {
				Msg::SC_CastleBuildingBase msg;
				msg.mapObjectUuid        = mapObjectUuid.str();
				msg.buildingBaseId       = obj->get_buildingBaseId();
				msg.buildingId           = obj->get_buildingId();
				msg.buildingLevel        = obj->get_buildingLevel();
				msg.mission              = obj->get_mission();
				msg.missionDuration      = obj->get_missionDuration();
				msg.missionParam2        = obj->get_missionParam2();
				msg.missionTimeBegin     = obj->get_missionTimeBegin();
				msg.missionTimeRemaining = saturatedSub(obj->get_missionTimeEnd(), Poseidon::getUtcTime());
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->forceShutdown();
			}
		}
	}
	void synchronizeTechWithClient(const MapObjectUuid &mapObjectUuid, const AccountUuid &ownerUuid,
		const boost::shared_ptr<MySql::Center_CastleTech> &obj)
	{
		PROFILE_ME;

		const auto session = PlayerSessionMap::get(ownerUuid);
		if(session){
			try {
				Msg::SC_CastleTech msg;
				msg.mapObjectUuid        = mapObjectUuid.str();
				msg.techId               = obj->get_techId();
				msg.techLevel            = obj->get_techLevel();
				msg.mission              = obj->get_mission();
				msg.missionDuration      = obj->get_missionDuration();
				msg.missionParam2        = obj->get_missionParam2();
				msg.missionTimeBegin     = obj->get_missionTimeBegin();
				msg.missionTimeRemaining = saturatedSub(obj->get_missionTimeEnd(), Poseidon::getUtcTime());
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->forceShutdown();
			}
		}
	}
	void synchronizeResourceWithClient(const MapObjectUuid &mapObjectUuid, const AccountUuid &ownerUuid,
		const boost::shared_ptr<MySql::Center_CastleResource> &obj)
	{
		PROFILE_ME;

		const auto session = PlayerSessionMap::get(ownerUuid);
		if(session){
			try {
				Msg::SC_CastleResource msg;
				msg.mapObjectUuid = mapObjectUuid.str();
				msg.resourceId    = obj->get_resourceId();
				msg.count         = obj->get_count();
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->forceShutdown();
			}
		}
	}

	void checkBuildingMission(const MapObjectUuid &mapObjectUuid, const AccountUuid &ownerUuid,
		const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj, boost::uint64_t utcNow)
	{
		PROFILE_ME;

		const auto mission = Castle::Mission(obj->get_mission());
		if(mission == Castle::MIS_NONE){
			return;
		}
		const auto missionTimeEnd = obj->get_missionTimeEnd();
		if(utcNow < missionTimeEnd){
			return;
		}

		LOG_EMPERY_CENTER_DEBUG("Building mission complete: mapObjectUuid = ", mapObjectUuid,
			", buildingBaseId = ", obj->get_buildingBaseId(), ", buildingId = ", obj->get_buildingId(), ", mission = ", (unsigned)mission);
		switch(mission){
		case Castle::MIS_CONSTRUCT:
			obj->set_buildingLevel(1);
			break;

		case Castle::MIS_UPGRADE:
			{
				const unsigned level = obj->get_buildingLevel();
				obj->set_buildingLevel(level + 1);
			}
			break;

		case Castle::MIS_DESTRUCT:
			obj->set_buildingId(0);
			obj->set_buildingLevel(0);
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown building mission: mapObjectUuid = ", mapObjectUuid,
				", buildingBaseId = ", obj->get_buildingBaseId(), ", mission = ", (unsigned)mission);
			DEBUG_THROW(Exception, sslit("Unknown building mission"));
		}

		obj->set_mission(Castle::MIS_NONE);
		obj->set_missionDuration(0);
		obj->set_missionParam2(0);
		obj->set_missionTimeEnd(utcNow + 1);

		synchronizeBuildingBaseWithClient(mapObjectUuid, ownerUuid, obj);
	}
	void checkTechMission(const MapObjectUuid &mapObjectUuid, const AccountUuid &ownerUuid,
		const boost::shared_ptr<MySql::Center_CastleTech> &obj, boost::uint64_t utcNow)
	{
		PROFILE_ME;

		const auto mission = Castle::Mission(obj->get_mission());
		if(mission == Castle::MIS_NONE){
			return;
		}
		const auto missionTimeEnd = obj->get_missionTimeEnd();
		if(utcNow < missionTimeEnd){
			return;
		}

		LOG_EMPERY_CENTER_DEBUG("Tech mission complete: mapObjectUuid = ", mapObjectUuid,
			", techId = ", obj->get_techId(), ", mission = ", (unsigned)mission);
		switch(mission){
//		case Castle::MIS_CONSTRUCT:
//			obj->set_techLevel(1);
//			break;

		case Castle::MIS_UPGRADE:
			{
				const unsigned level = obj->get_techLevel();
				obj->set_techLevel(level + 1);
			}
			break;

//		case Castle::MIS_DESTRUCT:
//			obj->set_techId(0);
//			obj->set_techLevel(0);
//			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown tech mission: mapObjectUuid = ", mapObjectUuid,
				", techId = ", obj->get_techId(), ", mission = ", (unsigned)mission);
			DEBUG_THROW(Exception, sslit("Unknown tech mission"));
		}

		obj->set_mission(Castle::MIS_NONE);
		obj->set_missionDuration(0);
		obj->set_missionParam2(0);
		obj->set_missionTimeEnd(utcNow + 1);

		synchronizeTechWithClient(mapObjectUuid, ownerUuid, obj);
	}
}

Castle::Castle(MapObjectTypeId mapObjectTypeId, const AccountUuid &ownerUuid, std::string name)
	: MapObject(mapObjectTypeId, ownerUuid, std::move(name))
{
	createInitBuildings(getMapObjectUuid(), m_buildings);
}
Castle::Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes)
	: MapObject(std::move(obj), attributes)
{
	createInitBuildings(getMapObjectUuid(), m_buildings);
}
Castle::Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
	const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
	const std::vector<boost::shared_ptr<MySql::Center_CastleTech>> &techs,
	const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources)
	: MapObject(std::move(obj), attributes)
{
	for(auto it = buildings.begin(); it != buildings.end(); ++it){
		const auto &obj = *it;
		m_buildings.emplace(BuildingBaseId(obj->get_buildingBaseId()), obj);
	}
	for(auto it = techs.begin(); it != techs.end(); ++it){
		const auto &obj = *it;
		m_techs.emplace(TechId(obj->get_techId()), obj);
	}
	for(auto it = resources.begin(); it != resources.end(); ++it){
		const auto &obj = *it;
		m_resources.emplace(ResourceId(obj->get_resourceId()), obj);
	}

	createInitBuildings(getMapObjectUuid(), m_buildings);
}
Castle::~Castle(){
}

void Castle::pumpStatus(bool forceSynchronizationWithClient){
	PROFILE_ME;

	const auto utcNow = Poseidon::getUtcTime();
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		checkBuildingMission(getMapObjectUuid(), getOwnerUuid(), it->second, utcNow);

		if(forceSynchronizationWithClient){
			synchronizeBuildingBaseWithClient(getMapObjectUuid(), getOwnerUuid(), it->second);
		}
	}
	for(auto it = m_techs.begin(); it != m_techs.end(); ++it){
		checkTechMission(getMapObjectUuid(), getOwnerUuid(), it->second, utcNow);

		if(forceSynchronizationWithClient){
			synchronizeTechWithClient(getMapObjectUuid(), getOwnerUuid(), it->second);
		}
	}
	for(auto it = m_resources.begin(); it != m_resources.end(); ++it){
		// 城内产出。
		if(forceSynchronizationWithClient){
			synchronizeResourceWithClient(getMapObjectUuid(), getOwnerUuid(), it->second);
		}
	}
}
void Castle::pumpBuildingStatus(BuildingBaseId buildingBaseId, bool forceSynchronizationWithClient){
	PROFILE_ME;

	const auto it = m_buildings.find(buildingBaseId);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_DEBUG("Building base not found: mapObjectUuid = ", getMapObjectUuid(), ", buildingBaseId = ", buildingBaseId);
		return;
	}

	const auto utcNow = Poseidon::getUtcTime();
	checkBuildingMission(getMapObjectUuid(), getOwnerUuid(), it->second, utcNow);

	if(forceSynchronizationWithClient){
		synchronizeBuildingBaseWithClient(getMapObjectUuid(), getOwnerUuid(), it->second);
	}
}
void Castle::pumpTechStatus(TechId techId, bool forceSynchronizationWithClient){
	PROFILE_ME;

	const auto it = m_techs.find(techId);
	if(it == m_techs.end()){
		LOG_EMPERY_CENTER_DEBUG("Tech not found: mapObjectUuid = ", getMapObjectUuid(), ", techId = ", techId);
		return;
	}

	const auto utcNow = Poseidon::getUtcTime();
	checkTechMission(getMapObjectUuid(), getOwnerUuid(), it->second, utcNow);

	if(forceSynchronizationWithClient){
		synchronizeTechWithClient(getMapObjectUuid(), getOwnerUuid(), it->second);
	}
}

Castle::BuildingInfo Castle::getBuilding(BuildingBaseId buildingBaseId) const {
	PROFILE_ME;

	BuildingInfo info = { };
	info.buildingBaseId = buildingBaseId;

	const auto it = m_buildings.find(buildingBaseId);
	if(it == m_buildings.end()){
		return info;
	}
	fillBuildingInfo(info, it->second);
	return info;
}
void Castle::getAllBuildings(std::vector<Castle::BuildingInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_buildings.size());
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		BuildingInfo info;
		fillBuildingInfo(info, it->second);
		ret.emplace_back(std::move(info));
	}
}
std::size_t Castle::countBuildingsById(BuildingId buildingId) const {
	PROFILE_ME;

	std::size_t count = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		if(BuildingId(it->second->get_buildingId()) != buildingId){
			continue;
		}
		++count;
	}
	return count;
}
void Castle::getBuildingsById(std::vector<Castle::BuildingInfo> &ret, BuildingId buildingId) const {
	PROFILE_ME;

	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		if(BuildingId(it->second->get_buildingId()) != buildingId){
			continue;
		}
		BuildingInfo info;
		fillBuildingInfo(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

void Castle::createBuildingMission(BuildingBaseId buildingBaseId, Castle::Mission mission, BuildingId buildingId){
	PROFILE_ME;

	auto it = m_buildings.find(buildingBaseId);
	if(it == m_buildings.end()){
		auto obj = boost::make_shared<MySql::Center_CastleBuildingBase>(
			getMapObjectUuid().get(), buildingBaseId.get(), 0, 0, MIS_NONE, 0, 0, 0, 0);
		obj->asyncSave(true);
		it = m_buildings.emplace(buildingBaseId, obj).first;
	}
	const auto &obj = it->second;
	const auto oldMission = Mission(obj->get_mission());
	if(oldMission != MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("Building mission conflict: mapObjectUuid = ", getMapObjectUuid(), ", buildingBaseId = ", buildingBaseId);
		DEBUG_THROW(Exception, sslit("Building mission conflict"));
	}

	boost::uint64_t duration;
	switch(mission){
	case MIS_CONSTRUCT:
		{
			const auto buildingData = Data::CastleBuilding::require(buildingId);
			const auto upgradeData = Data::CastleUpgradeAbstract::require(buildingData->type, 1);
			duration = upgradeData->upgradeDuration;
		}
		obj->set_buildingId(buildingId.get());
		obj->set_buildingLevel(0);
		break;

	case MIS_UPGRADE:
		{
			const unsigned level = obj->get_buildingLevel();
			const auto buildingData = Data::CastleBuilding::require(BuildingId(obj->get_buildingId()));
			const auto upgradeData = Data::CastleUpgradeAbstract::require(buildingData->type, level + 1);
			duration = upgradeData->upgradeDuration;
		}
		break;

	case MIS_DESTRUCT:
		{
			const unsigned level = obj->get_buildingLevel();
			const auto buildingData = Data::CastleBuilding::require(BuildingId(obj->get_buildingId()));
			const auto upgradeData = Data::CastleUpgradeAbstract::require(buildingData->type, level);
			duration = upgradeData->destructDuration;
		}
		break;

	default:
		LOG_EMPERY_CENTER_ERROR("Unknown building mission: mapObjectUuid = ", getMapObjectUuid(), ", buildingBaseId = ", buildingBaseId,
			", mission = ", (unsigned)mission);
		DEBUG_THROW(Exception, sslit("Unknown building mission"));
	}

	const auto utcNow = Poseidon::getUtcTime();

	obj->set_mission(mission);
	obj->set_missionDuration(duration);
	// obj->set_missionParam2(0);
	obj->set_missionTimeBegin(utcNow);
	obj->set_missionTimeEnd(saturatedAdd(utcNow, duration));

	checkBuildingMission(getMapObjectUuid(), getOwnerUuid(), obj, utcNow);
	synchronizeBuildingBaseWithClient(getMapObjectUuid(), getOwnerUuid(), obj);
}
void Castle::cancelBuildingMission(BuildingBaseId buildingBaseId){
	PROFILE_ME;

	const auto it = m_buildings.find(buildingBaseId);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_DEBUG("Building base not found: mapObjectUuid = ", getMapObjectUuid(), ", buildingBaseId = ", buildingBaseId);
		return;
	}
	const auto &obj = it->second;
	const auto oldMission = Mission(obj->get_mission());
	if(oldMission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No building mission: mapObjectUuid = ", getMapObjectUuid(), ", buildingBaseId = ", buildingBaseId);
		return;
	}

	const auto utcNow = Poseidon::getUtcTime();

	obj->set_mission(MIS_NONE);
	obj->set_missionDuration(0);
	obj->set_missionParam2(0);
	obj->set_missionTimeBegin(utcNow);
	obj->set_missionTimeEnd(utcNow);

	checkBuildingMission(getMapObjectUuid(), getOwnerUuid(), obj, utcNow);
	synchronizeBuildingBaseWithClient(getMapObjectUuid(), getOwnerUuid(), obj);
}
void Castle::speedUpBuildingMission(BuildingBaseId buildingBaseId, boost::uint64_t deltaDuration){
	PROFILE_ME;

	const auto it = m_buildings.find(buildingBaseId);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_WARNING("Building base not found: mapObjectUuid = ", getMapObjectUuid(), ", buildingBaseId = ", buildingBaseId);
		DEBUG_THROW(Exception, sslit("Building base not found"));
	}
	const auto &obj = it->second;
	const auto oldMission = Mission(obj->get_mission());
	if(oldMission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No building mission: mapObjectUuid = ", getMapObjectUuid(), ", buildingBaseId = ", buildingBaseId);
		DEBUG_THROW(Exception, sslit("No building mission"));
	}

	const auto utcNow = Poseidon::getUtcTime();

	obj->set_missionTimeEnd(saturatedSub(obj->get_missionTimeEnd(), deltaDuration));

	checkBuildingMission(getMapObjectUuid(), getOwnerUuid(), obj, utcNow);
	synchronizeBuildingBaseWithClient(getMapObjectUuid(), getOwnerUuid(), obj);
}

Castle::TechInfo Castle::getTech(TechId techId) const {
	PROFILE_ME;

	TechInfo info = { };
	info.techId = techId;

	const auto it = m_techs.find(techId);
	if(it == m_techs.end()){
		return info;
	}
	fillTechInfo(info, it->second);
	return info;
}
void Castle::getAllTechs(std::vector<Castle::TechInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_techs.size());
	for(auto it = m_techs.begin(); it != m_techs.end(); ++it){
		TechInfo info;
		fillTechInfo(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

void Castle::createTechMission(TechId techId, Castle::Mission mission){
	PROFILE_ME;

	auto it = m_techs.find(techId);
	if(it == m_techs.end()){
		auto obj = boost::make_shared<MySql::Center_CastleTech>(
			getMapObjectUuid().get(), techId.get(), 0, MIS_NONE, 0, 0, 0, 0);
		obj->asyncSave(true);
		it = m_techs.emplace(techId, obj).first;
	}
	const auto &obj = it->second;
	const auto oldMission = Mission(obj->get_mission());
	if(oldMission != MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("Tech mission conflict: mapObjectUuid = ", getMapObjectUuid(), ", techId = ", techId);
		DEBUG_THROW(Exception, sslit("Tech mission conflict"));
	}

	boost::uint64_t duration;
	switch(mission){
//	case MIS_CONSTRUCT:
//		break;

	case MIS_UPGRADE:
		{
			const unsigned level = obj->get_techLevel();
			const auto techData = Data::CastleTech::require(TechId(obj->get_techId()), level + 1);
			duration = techData->upgradeDuration;
		}
		break;

//	case MIS_DESTRUCT:
//		break;

	default:
		LOG_EMPERY_CENTER_ERROR("Unknown tech mission: mapObjectUuid = ", getMapObjectUuid(), ", techId = ", techId,
			", mission = ", (unsigned)mission);
		DEBUG_THROW(Exception, sslit("Unknown tech mission"));
	}

	const auto utcNow = Poseidon::getUtcTime();

	obj->set_mission(mission);
	obj->set_missionDuration(duration);
	// obj->set_missionParam2(0);
	obj->set_missionTimeBegin(utcNow);
	obj->set_missionTimeEnd(saturatedAdd(utcNow, duration));

	checkTechMission(getMapObjectUuid(), getOwnerUuid(), obj, utcNow);
	synchronizeTechWithClient(getMapObjectUuid(), getOwnerUuid(), obj);
}
void Castle::cancelTechMission(TechId techId){
	PROFILE_ME;

	const auto it = m_techs.find(techId);
	if(it == m_techs.end()){
		LOG_EMPERY_CENTER_DEBUG("Tech not found: mapObjectUuid = ", getMapObjectUuid(), ", techId = ", techId);
		return;
	}
	const auto &obj = it->second;
	const auto oldMission = Mission(obj->get_mission());
	if(oldMission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No tech mission: mapObjectUuid = ", getMapObjectUuid(), ", techId = ", techId);
		return;
	}

	const auto utcNow = Poseidon::getUtcTime();

	obj->set_mission(MIS_NONE);
	obj->set_missionDuration(0);
	obj->set_missionParam2(0);
	obj->set_missionTimeBegin(utcNow);
	obj->set_missionTimeEnd(utcNow);

	checkTechMission(getMapObjectUuid(), getOwnerUuid(), obj, utcNow);
	synchronizeTechWithClient(getMapObjectUuid(), getOwnerUuid(), obj);
}
void Castle::speedUpTechMission(TechId techId, boost::uint64_t deltaDuration){
	PROFILE_ME;

	const auto it = m_techs.find(techId);
	if(it == m_techs.end()){
		LOG_EMPERY_CENTER_WARNING("Tech not found: mapObjectUuid = ", getMapObjectUuid(), ", techId = ", techId);
		DEBUG_THROW(Exception, sslit("Tech not found"));
	}
	const auto &obj = it->second;
	const auto oldMission = Mission(obj->get_mission());
	if(oldMission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No tech mission: mapObjectUuid = ", getMapObjectUuid(), ", techId = ", techId);
		DEBUG_THROW(Exception, sslit("No tech mission"));
	}

	const auto utcNow = Poseidon::getUtcTime();

	obj->set_missionTimeEnd(saturatedSub(obj->get_missionTimeEnd(), deltaDuration));

	checkTechMission(getMapObjectUuid(), getOwnerUuid(), obj, utcNow);
	synchronizeTechWithClient(getMapObjectUuid(), getOwnerUuid(), obj);
}

Castle::ResourceInfo Castle::getResource(ResourceId resourceId) const {
	PROFILE_ME;

	ResourceInfo info = { };
	info.resourceId = resourceId;

	const auto it = m_resources.find(resourceId);
	if(it == m_resources.end()){
		return info;
	}
	fillResourceInfo(info, it->second);
	return info;
}
void Castle::getAllResources(std::vector<Castle::ResourceInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_resources.size());
	for(auto it = m_resources.begin(); it != m_resources.end(); ++it){
		ResourceInfo info;
		fillResourceInfo(info, it->second);
		ret.emplace_back(std::move(info));
	}
}
ResourceId Castle::commitResourceTransactionNoThrow(const Castle::ResourceTransactionElement *elements, std::size_t count,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	boost::shared_ptr<bool> withdrawn;

	boost::container::flat_map<boost::shared_ptr<MySql::Center_CastleResource>,
		boost::uint64_t /* newCount */> tempResultMap;

	for(std::size_t i = 0; i < count; ++i){
		const auto operation  = elements[i].m_operation;
		const auto resourceId = elements[i].m_resourceId;
		const auto deltaCount = elements[i].m_deltaCount;

		if(deltaCount == 0){
			continue;
		}

		const auto reason = elements[i].m_reason;
		const auto param1 = elements[i].m_param1;
		const auto param2 = elements[i].m_param2;
		const auto param3 = elements[i].m_param3;

		switch(operation){
		case ResourceTransactionElement::OP_NONE:
			break;

		case ResourceTransactionElement::OP_ADD:
			{
				boost::shared_ptr<MySql::Center_CastleResource> obj;
				{
					const auto it = m_resources.find(resourceId);
					if(it == m_resources.end()){
						obj = boost::make_shared<MySql::Center_CastleResource>(
							getMapObjectUuid().get(), resourceId.get(), 0);
						obj->asyncSave(true);
						m_resources.emplace(resourceId, obj);
					} else {
						obj = it->second;
					}
				}
				auto tempIt = tempResultMap.find(obj);
				if(tempIt == tempResultMap.end()){
					tempIt = tempResultMap.emplace_hint(tempIt, obj, obj->get_count());
				}
				const auto oldCount = tempIt->second;
				tempIt->second = checkedAdd(oldCount, deltaCount);
				const auto newCount = tempIt->second;

				const auto &mapObjectUuid = getMapObjectUuid();
				const auto &ownerUuid = getOwnerUuid();
				LOG_EMPERY_CENTER_DEBUG("@ Resource transaction: add: mapObjectUuid = ", mapObjectUuid, ", ownerUuid = ", ownerUuid,
					", resourceId = ", resourceId, ", oldCount = ", oldCount, ", deltaCount = ", deltaCount, ", newCount = ", newCount,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				if(!withdrawn){
					withdrawn = boost::make_shared<bool>(true);
				}
				Poseidon::asyncRaiseEvent(
					boost::make_shared<Events::ResourceChanged>(mapObjectUuid, ownerUuid,
						resourceId, oldCount, newCount, reason, param1, param2, param3),
					withdrawn);
			}
			break;

		case ResourceTransactionElement::OP_REMOVE:
		case ResourceTransactionElement::OP_REMOVE_SATURATED:
			{
				const auto it = m_resources.find(resourceId);
				if(it == m_resources.end()){
					if(operation != ResourceTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("Resource not found: resourceId = ", resourceId);
						return resourceId;
					}
					break;
				}
				const auto &obj = it->second;
				auto tempIt = tempResultMap.find(obj);
				if(tempIt == tempResultMap.end()){
					tempIt = tempResultMap.emplace_hint(tempIt, obj, obj->get_count());
				}
				const auto oldCount = tempIt->second;
				if(tempIt->second >= deltaCount){
					tempIt->second -= deltaCount;
				} else {
					if(operation != ResourceTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("No enough resources: resourceId = ", resourceId,
							", tempCount = ", tempIt->second, ", deltaCount = ", deltaCount);
						return resourceId;
					}
					tempIt->second = 0;
				}
				const auto newCount = tempIt->second;

				const auto &mapObjectUuid = getMapObjectUuid();
				const auto &ownerUuid = getOwnerUuid();
				LOG_EMPERY_CENTER_DEBUG("@ Resource transaction: remove: mapObjectUuid = ", mapObjectUuid, ", ownerUuid = ", ownerUuid,
					", resourceId = ", resourceId, ", oldCount = ", oldCount, ", deltaCount = ", deltaCount, ", newCount = ", newCount,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				if(!withdrawn){
					withdrawn = boost::make_shared<bool>(true);
				}
				Poseidon::asyncRaiseEvent(
					boost::make_shared<Events::ResourceChanged>(mapObjectUuid, ownerUuid,
						resourceId, oldCount, newCount, reason, param1, param2, param3),
					withdrawn);
			}
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown resource transaction operation: operation = ", (unsigned)operation);
			DEBUG_THROW(Exception, sslit("Unknown resource transaction operation"));
		}
	}

	if(callback){
		callback();
	}

	for(auto it = tempResultMap.begin(); it != tempResultMap.end(); ++it){
		it->first->set_count(it->second);
	}
	if(withdrawn){
		*withdrawn = false;
	}

	for(auto it = tempResultMap.begin(); it != tempResultMap.end(); ++it){
		synchronizeResourceWithClient(getMapObjectUuid(), getOwnerUuid(), it->first);
	}

	return ResourceId();
}
void Castle::commitResourceTransaction(const Castle::ResourceTransactionElement *elements, std::size_t count,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	const auto insuffId = commitResourceTransactionNoThrow(elements, count, callback);
	if(insuffId != ResourceId()){
		LOG_EMPERY_CENTER_DEBUG("Insufficient resources in castle: mapObjectUuid = ", getMapObjectUuid(), ", insuffId = ", insuffId);
		DEBUG_THROW(Exception, sslit("Insufficient resources in castle"));
	}
}

}
