#include "precompiled.hpp"
#include "castle.hpp"
#include "map_object.hpp"
#include "mysql/castle_building_base.hpp"
#include "mysql/castle_resource.hpp"
#include "msg/sc_castle.hpp"
#include "singletons/player_session_map.hpp"
#include "checked_arithmetic.hpp"
#include "player_session.hpp"

namespace EmperyCenter {

namespace {
	void fillBuildingInfo(Castle::BuildingInfo &info, const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj){
		PROFILE_ME;

		info.baseIndex        = obj->get_baseIndex();
		info.buildingId       = BuildingId(obj->get_buildingId());
		info.buildingLevel    = obj->get_buildingLevel();
		info.mission          = Castle::Mission(obj->get_mission());
		info.missionParam1    = obj->get_missionParam1();
		info.missionParam2    = obj->get_missionParam2();
		info.missionTimeBegin = obj->get_missionTimeBegin();
		info.missionTimeEnd   = obj->get_missionTimeEnd();
	}
	void fillResourceInfo(Castle::ResourceInfo &info, const boost::shared_ptr<MySql::Center_CastleResource> &obj){
		PROFILE_ME;

		info.resourceId       = ResourceId(obj->get_resourceId());
		info.count            = obj->get_count();
	}

	void synchronizeBuildingBase(const Castle *castle, const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj){
		PROFILE_ME;

		const auto session = PlayerSessionMap::get(castle->getOwnerUuid());
		if(session){
			try {
				Msg::SC_CastleBuildingBase msg;
				msg.mapObjectUuid        = castle->getMapObjectUuid().str();
				msg.baseIndex            = obj->get_baseIndex();
				msg.buildingId           = obj->get_buildingId();
				msg.buildingLevel        = obj->get_buildingLevel();
				msg.mission              = obj->get_mission();
				msg.missionParam1        = obj->get_missionParam1();
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
}

Castle::Castle(MapObjectTypeId mapObjectTypeId, const AccountUuid &ownerUuid)
	: MapObject(mapObjectTypeId, ownerUuid)
{
}
Castle::Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
	const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
	const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources)
	: MapObject(std::move(obj), attributes)
{
	for(auto it = buildings.begin(); it != buildings.end(); ++it){
		const auto &obj = *it;
		m_buildings.emplace(obj->get_baseIndex(), obj);
	}
	for(auto it = resources.begin(); it != resources.end(); ++it){
		const auto &obj = *it;
		m_resources.emplace(ResourceId(obj->get_resourceId()), obj);
	}
}
Castle::~Castle(){
}

void Castle::pumpStatus(){
	PROFILE_ME;

	const auto utcNow = Poseidon::getUtcTime();

	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto &obj = it->second;
		if(obj->get_mission() == MIS_NONE){
			continue;
		}
		if(utcNow < it->second->get_missionTimeEnd()){
			continue;
		}
		completeMission(it->first);
	}
}

Castle::BuildingInfo Castle::getBuilding(unsigned baseIndex) const {
	PROFILE_ME;

	BuildingInfo info = { };
	info.baseIndex = baseIndex;

	const auto it = m_buildings.find(baseIndex);
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

void Castle::createMission(unsigned baseIndex, Castle::Mission mission, BuildingId buildingId,
	boost::uint64_t missionParam1, boost::uint64_t missionParam2, boost::uint64_t duration)
{
	PROFILE_ME;

	auto it = m_buildings.find(baseIndex);
	if(it == m_buildings.end()){
		auto obj = boost::make_shared<MySql::Center_CastleBuildingBase>(
			getMapObjectUuid().get(), baseIndex, 0, 0, MIS_NONE, 0, 0, 0, 0);
		obj->asyncSave(true);
		it = m_buildings.emplace(baseIndex, obj).first;
	}
	const auto &obj = it->second;

	const auto utcNow = Poseidon::getUtcTime();
	const auto missionTimeEnd = checkedAdd(utcNow, duration);

(void)missionParam1, (void)missionParam2;
	switch(mission){
	case MIS_CONSTRUCT:
		// TODO 扣资源。
		obj->set_buildingId(buildingId.get());
		obj->set_buildingLevel(0);
		break;

	case MIS_UPGRADE:
		// TODO 扣资源。
		break;

	case MIS_DESTRUCT:
		// TODO 扣资源。
		break;

	case MIS_PRODUCE:
		break;

	default:
		LOG_EMPERY_CENTER_ERROR("Unknown mission: mapObjectUuid = ", getMapObjectUuid(), ", baseIndex = ", baseIndex,
			", mission = ", (unsigned)mission);
		DEBUG_THROW(Exception, sslit("Unknown mission"));
	}

	obj->set_mission(MIS_NONE);
	obj->set_missionParam1(missionParam1);
	obj->set_missionParam2(missionParam2);
	obj->set_missionTimeBegin(utcNow);
	obj->set_missionTimeEnd(missionTimeEnd);
}
void Castle::cancelMission(unsigned baseIndex){
	PROFILE_ME;

	const auto it = m_buildings.find(baseIndex);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_DEBUG("Building base not found: mapObjectUuid = ", getMapObjectUuid(), ", baseIndex = ", baseIndex);
		return;
	}
	const auto &obj = it->second;
	const auto mission = Mission(obj->get_mission());
	if(mission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No mission on building base: mapObjectUuid = ", getMapObjectUuid(), ", baseIndex = ", baseIndex);
		return;
	}
	const auto missionParam1 = obj->get_missionParam1();
	const auto missionParam2 = obj->get_missionParam2();

(void)missionParam1, (void)missionParam2;
	switch(mission){
	case MIS_CONSTRUCT:
		break;

	case MIS_UPGRADE:
		break;

	case MIS_DESTRUCT:
		break;

	case MIS_PRODUCE:
		break;

	default:
		LOG_EMPERY_CENTER_ERROR("Unknown mission: mapObjectUuid = ", getMapObjectUuid(), ", baseIndex = ", baseIndex,
			", mission = ", (unsigned)mission);
	}

	obj->set_mission(MIS_NONE);
	obj->set_missionParam1(0);
	obj->set_missionParam2(0);
	obj->set_missionTimeBegin(0);
	obj->set_missionTimeEnd(0);
}
void Castle::completeMission(unsigned baseIndex){
	PROFILE_ME;

	const auto it = m_buildings.find(baseIndex);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_WARNING("Building base not found: mapObjectUuid = ", getMapObjectUuid(), ", baseIndex = ", baseIndex);
		DEBUG_THROW(Exception, sslit("Building base not found"));
	}
	const auto &obj = it->second;
	const auto mission = Mission(obj->get_mission());
	if(mission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No mission on building base: mapObjectUuid = ", getMapObjectUuid(), ", baseIndex = ", baseIndex);
		DEBUG_THROW(Exception, sslit("Building base not found"));
	}
	const auto missionParam1 = obj->get_missionParam1();
	const auto missionParam2 = obj->get_missionParam2();

(void)missionParam1, (void)missionParam2;
	switch(mission){
	case MIS_CONSTRUCT:
		break;

	case MIS_UPGRADE:
		obj->set_buildingLevel(obj->get_buildingLevel() + 1);
		break;

	case MIS_DESTRUCT:
		obj->set_buildingId(0);
		obj->set_buildingLevel(0);
		break;

	case MIS_PRODUCE:
		break;

	default:
		LOG_EMPERY_CENTER_ERROR("Unknown mission: mapObjectUuid = ", getMapObjectUuid(), ", baseIndex = ", baseIndex,
			", mission = ", (unsigned)mission);
		DEBUG_THROW(Exception, sslit("Unknown mission"));
	}

	obj->set_mission(MIS_NONE);
	obj->set_missionParam1(0);
	obj->set_missionParam2(0);
	obj->set_missionTimeBegin(0);
	obj->set_missionTimeEnd(0);
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
ResourceId Castle::commitResourceTransactionNoThrow(const Castle::ResourceTransactionElement *elements, std::size_t count){
	PROFILE_ME;

	if(count == 0){
		return ResourceId();
	}

	boost::container::flat_map<boost::shared_ptr<MySql::Center_CastleResource>,
		boost::uint64_t /* newCount */> tempResultMap;

	for(std::size_t i = 0; i < count; ++i){
		const auto operation  = elements[i].operation;
		const auto resourceId = elements[i].resourceId;
		const auto deltaCount = elements[i].deltaCount;

		if(deltaCount == 0){
			continue;
		}

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
				auto &tempCount = tempResultMap[obj];
				tempCount = checkedAdd(tempCount, deltaCount);
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
				auto &tempCount = tempResultMap[obj];
				if(tempCount >= deltaCount){
					tempCount -= deltaCount;
				} else {
					if(operation != ResourceTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("No enough resources: resourceId = ", resourceId,
							", tempCount = ", tempCount, ", deltaCount = ", deltaCount);
						return resourceId;
					}
					tempCount = 0;
				}
			}
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown resource transaction operation: operation = ", (unsigned)operation);
			DEBUG_THROW(Exception, sslit("Unknown resource transaction operation"));
		}
	}

	for(auto it = tempResultMap.begin(); it != tempResultMap.end(); ++it){
		it->first->set_count(it->second);
	}

	const auto session = PlayerSessionMap::get(getOwnerUuid());
	if(session){
		try {
			for(auto it = tempResultMap.begin(); it != tempResultMap.end(); ++it){
				Msg::SC_CastleResource msg;
				msg.mapObjectUuid    = getMapObjectUuid().str();
				msg.resourceId       = it->first->get_resourceId();
				msg.count            = it->first->get_count();
				session->send(msg);
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->forceShutdown();
		}
	}

	return ResourceId();
}
void Castle::commitResourceTransaction(const Castle::ResourceTransactionElement *elements, std::size_t count){
	PROFILE_ME;

	const auto insuffResourceId = commitResourceTransactionNoThrow(elements, count);
	if(insuffResourceId != ResourceId()){
		LOG_EMPERY_CENTER_DEBUG("Insufficient resources in castle: mapObjectUuid = ", getMapObjectUuid(),
			", insuffResourceId = ", insuffResourceId);
		DEBUG_THROW(Exception, sslit("Insufficient resources in castle"));
	}
}

}
