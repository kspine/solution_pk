#ifndef EMPERY_CENTER_CASTLE_HPP_
#define EMPERY_CENTER_CASTLE_HPP_

#include <poseidon/cxx_util.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <vector>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_CastleBuildingBase;
	class Center_CastleResource;
}

class MapObject;

class Castle : NONCOPYABLE {
public:
	enum Mission {
		MIS_NONE            = 0,
		MIS_CONSTRUCTING    = 1,
		MIS_UPGRADING       = 2,
		MIS_DESTRUCTING     = 3,
	};

	struct BuildingInfo {
		unsigned baseIndex;
		BuildingId buildingId;
		unsigned buildingLevel;
		Mission mission;
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
	const boost::shared_ptr<MapObject> m_mapObject;

	boost::container::flat_map<unsigned, boost::shared_ptr<MySql::Center_CastleBuildingBase>> m_buildings;
	boost::container::flat_map<ResourceId, boost::shared_ptr<MySql::Center_CastleResource>> m_resources;

public:
	explicit Castle(boost::shared_ptr<MapObject> mapObject);
	Castle(boost::shared_ptr<MapObject> mapObject,
		const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
		const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources);
	~Castle();

public:
	MapObjectUuid getMapObjectUuid() const;

	BuildingInfo getBuilding(unsigned baseIndex) const;
	void getAllBuildings(std::vector<BuildingInfo> &ret) const;
	void updateBuilding(BuildingInfo info);

	ResourceInfo getResource(ResourceId resourceId) const;
	void getAllResources(std::vector<ResourceInfo> &ret) const;
	ResourceId commitResourceTransactionNoThrow(const ResourceTransactionElement *elements, std::size_t count);
	void commitResourceTransaction(const ResourceTransactionElement *elements, std::size_t count);
};

}

#endif
