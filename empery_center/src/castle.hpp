#ifndef EMPERY_CENTER_CASTLE_HPP_
#define EMPERY_CENTER_CASTLE_HPP_

#include "map_object.hpp"
#include <poseidon/cxx_util.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <vector>

namespace EmperyCenter {

namespace MySql {
	class Center_CastleBuildingBase;
	class Center_CastleResource;
}

class Castle : public MapObject {
public:
	enum Mission {
		MIS_NONE            = 0,
		MIS_CONSTRUCT       = 1,
		MIS_UPGRADE         = 2,
		MIS_DESTRUCT        = 3,
		MIS_PRODUCE         = 4,
	};

	struct BuildingInfo {
		unsigned baseIndex;
		BuildingId buildingId;
		unsigned buildingLevel;
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

	struct ResourceTransactionElement {
		enum Operation {
			OP_NONE             = 0,
			OP_ADD              = 1,
			OP_REMOVE           = 2,
			OP_REMOVE_SATURATED = 3,
		};

		Operation operation;
		ResourceId resourceId;
		boost::uint64_t deltaCount;

		ResourceTransactionElement(Operation operation_, ResourceId resourceId_, boost::uint64_t deltaCount_)
			: operation(operation_), resourceId(resourceId_), deltaCount(deltaCount_)
		{
		}
	};

private:
	boost::container::flat_map<unsigned, boost::shared_ptr<MySql::Center_CastleBuildingBase>> m_buildings;
	boost::container::flat_map<ResourceId, boost::shared_ptr<MySql::Center_CastleResource>> m_resources;

public:
	Castle(MapObjectTypeId mapObjectTypeId, const AccountUuid &ownerUuid);
	Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
		const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
		const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources);
	~Castle();

public:
	void pumpStatus();

	BuildingInfo getBuilding(unsigned baseIndex) const;
	void getAllBuildings(std::vector<BuildingInfo> &ret) const;
	// 如果指定地基上有任务会抛出异常。
	void createMission(unsigned baseIndex, Mission mission, BuildingId buildingId,
		boost::uint64_t missionParam1, boost::uint64_t missionParam2, boost::uint64_t duration);
	void cancelMission(unsigned baseIndex);
	void completeMission(unsigned baseIndex);

	ResourceInfo getResource(ResourceId resourceId) const;
	void getAllResources(std::vector<ResourceInfo> &ret) const;
	ResourceId commitResourceTransactionNoThrow(const ResourceTransactionElement *elements, std::size_t count);
	void commitResourceTransaction(const ResourceTransactionElement *elements, std::size_t count);
};

}

#endif
