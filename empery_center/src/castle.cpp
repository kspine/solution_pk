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
		info.missionTimeBegin = obj->get_missionTimeBegin();
		info.missionTimeEnd   = obj->get_missionTimeEnd();
	}
	void fillResourceInfo(Castle::ResourceInfo &info, const boost::shared_ptr<MySql::Center_CastleResource> &obj){
		PROFILE_ME;

		info.resourceId       = ResourceId(obj->get_resourceId());
		info.count            = obj->get_count();
	}
}

Castle::Castle(boost::shared_ptr<MapObject> mapObject)
	: m_mapObject(std::move(mapObject))
{
}
Castle::Castle(boost::shared_ptr<MapObject> mapObject,
	const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
	const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources)
	: Castle(std::move(mapObject))
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

MapObjectUuid Castle::getMapObjectUuid() const {
	return m_mapObject->getMapObjectUuid();
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
void Castle::updateBuilding(Castle::BuildingInfo info){
	PROFILE_ME;

	boost::shared_ptr<MySql::Center_CastleBuildingBase> obj;
	{
		const auto it = m_buildings.find(info.baseIndex);
		if(it == m_buildings.end()){
			obj = boost::make_shared<MySql::Center_CastleBuildingBase>(
				m_mapObject->getMapObjectUuid().get(), info.baseIndex, 0, 0, 0, 0, 0);
			obj->asyncSave(true);
			m_buildings.emplace(info.baseIndex, obj);
		} else {
			obj = it->second;
		}
	}

	obj->set_buildingId      (info.buildingId.get());
	obj->set_buildingLevel   (info.buildingLevel);
	obj->set_mission         (static_cast<unsigned>(info.mission));
	obj->set_missionTimeBegin(info.missionTimeBegin);
	obj->set_missionTimeEnd  (info.missionTimeEnd);

	const auto session = PlayerSessionMap::get(m_mapObject->getOwnerUuid());
	if(session){
		try {
			Msg::SC_CastleBuildingBase msg;
			msg.mapObjectUuid        = m_mapObject->getMapObjectUuid().str();
			msg.baseIndex            = obj->get_baseIndex();
			msg.buildingId           = obj->get_buildingId();
			msg.buildingLevel        = obj->get_buildingLevel();
			msg.mission              = obj->get_mission();
			msg.missionTimeBegin     = obj->get_missionTimeBegin();
			msg.missionTimeRemaining = saturatedSub(obj->get_missionTimeEnd(), Poseidon::getLocalTime());
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->forceShutdown();
		}
	}
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
							m_mapObject->getMapObjectUuid().get(), resourceId.get(), 0);
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

	const auto session = PlayerSessionMap::get(m_mapObject->getOwnerUuid());
	if(session){
		try {
			for(auto it = tempResultMap.begin(); it != tempResultMap.end(); ++it){
				Msg::SC_CastleResource msg;
				msg.mapObjectUuid    = m_mapObject->getMapObjectUuid().str();
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
		LOG_EMPERY_CENTER_DEBUG("Insufficient resources in castle: mapObjectUuid = ", m_mapObject->getMapObjectUuid(),
			", insuffResourceId = ", insuffResourceId);
		DEBUG_THROW(Exception, sslit("Insufficient resources in castle"));
	}
}

}
