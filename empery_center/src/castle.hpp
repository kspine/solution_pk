#ifndef EMPERY_CENTER_CASTLE_HPP_
#define EMPERY_CENTER_CASTLE_HPP_

#include "map_object.hpp"
#include <poseidon/cxx_util.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/function.hpp>
#include <vector>

namespace EmperyCenter {

namespace MySql {
	class Center_CastleBuildingBase;
	class Center_CastleResource;
	class Center_CastleTech;
}

class Castle : public MapObject {
public:
	enum Mission {
		MIS_NONE            = 0,
		MIS_CONSTRUCT       = 1,
		MIS_UPGRADE         = 2,
		MIS_DESTRUCT        = 3,
//		MIS_PRODUCE         = 4,
	};

	struct BuildingInfo {
		BuildingBaseId buildingBaseId;
		BuildingId buildingId;
		unsigned buildingLevel;
		Mission mission;
		boost::uint64_t missionParam1;
		boost::uint64_t missionParam2;
		boost::uint64_t missionTimeBegin;
		boost::uint64_t missionTimeEnd;
	};

	struct TechInfo {
		TechId techId;
		unsigned techLevel;
		Mission mission;
		boost::uint64_t missionParam1;
		boost::uint64_t missionParam2;
		boost::uint64_t missionTimeBegin;
		boost::uint64_t missionTimeEnd;
	};

	struct ResourceInfo {
		ResourceId resourceId;
		boost::uint64_t count;
	};

	class ResourceTransactionElement {
		friend Castle;

	public:
		enum Operation {
			OP_NONE             = 0,
			OP_ADD              = 1,
			OP_REMOVE           = 2,
			OP_REMOVE_SATURATED = 3,
		};

	private:
		Operation m_operation;
		ResourceId m_resourceId;
		boost::uint64_t m_deltaCount;

		ReasonId m_reason;
		boost::uint64_t m_param1;
		boost::uint64_t m_param2;
		boost::uint64_t m_param3;

	public:
		ResourceTransactionElement(Operation operation, ResourceId resourceId, boost::uint64_t deltaCount,
			ReasonId reason, boost::uint64_t param1, boost::uint64_t param2, boost::uint64_t param3)
			: m_operation(operation), m_resourceId(resourceId), m_deltaCount(deltaCount)
			, m_reason(reason), m_param1(param1), m_param2(param2), m_param3(param3)
		{
		}
	};

private:
	boost::container::flat_map<BuildingBaseId,
		boost::shared_ptr<MySql::Center_CastleBuildingBase>> m_buildings;
	boost::container::flat_map<TechId,
		boost::shared_ptr<MySql::Center_CastleTech>> m_techs;
	boost::container::flat_map<ResourceId,
		boost::shared_ptr<MySql::Center_CastleResource>> m_resources;

public:
	Castle(MapObjectTypeId mapObjectTypeId, const AccountUuid &ownerUuid);
	Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
		const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
		const std::vector<boost::shared_ptr<MySql::Center_CastleTech>> &techs,
		const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources);
	~Castle();

private:
	void checkBuildingMission(const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj, boost::uint64_t utcNow);
	void checkTechMission(const boost::shared_ptr<MySql::Center_CastleTech> &obj, boost::uint64_t utcNow);

public:
	void pumpStatus(bool forceSynchronizationWithClient = false);
	void pumpBuildingStatus(BuildingBaseId buildingBaseId, bool forceSynchronizationWithClient = false);
	void pumpTechStatus(TechId techId, bool forceSynchronizationWithClient = false);

	BuildingInfo getBuilding(BuildingBaseId buildingBaseId) const;
	BuildingInfo getBuildingById(BuildingId buildingId) const;
	void getAllBuildings(std::vector<BuildingInfo> &ret) const;
	// 如果指定地基上有任务会抛出异常。
	void createBuildingMission(BuildingBaseId buildingBaseId, Mission mission, BuildingId buildingId = BuildingId());
	void cancelBuildingMission(BuildingBaseId buildingBaseId);
	void speedUpBuildingMission(BuildingBaseId buildingBaseId, boost::uint64_t deltaDuration);

	TechInfo getTech(TechId techId) const;
	void getAllTechs(std::vector<TechInfo> &ret) const;
	// 同上。
	void createTechMission(TechId techId, Mission mission);
	void cancelTechMission(TechId techId);
	void speedUpTechMission(TechId techId, boost::uint64_t deltaDuration);

	ResourceInfo getResource(ResourceId resourceId) const;
	void getAllResources(std::vector<ResourceInfo> &ret) const;
	ResourceId commitResourceTransactionNoThrow(const ResourceTransactionElement *elements, std::size_t count,
		const boost::function<void ()> &callback = boost::function<void ()>());
	void commitResourceTransaction(const ResourceTransactionElement *elements, std::size_t count,
		const boost::function<void ()> &callback = boost::function<void ()>());
};

}

#endif
