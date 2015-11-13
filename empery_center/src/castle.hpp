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
		BuildingBaseId building_base_id;
		BuildingId building_id;
		unsigned building_level;
		Mission mission;
		boost::uint64_t mission_duration;
		boost::uint64_t mission_param2;
		boost::uint64_t mission_time_begin;
		boost::uint64_t mission_time_end;
	};

	struct TechInfo {
		TechId tech_id;
		unsigned tech_level;
		Mission mission;
		boost::uint64_t mission_duration;
		boost::uint64_t mission_param2;
		boost::uint64_t mission_time_begin;
		boost::uint64_t mission_time_end;
	};

	struct ResourceInfo {
		ResourceId resource_id;
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
		ResourceId m_resource_id;
		boost::uint64_t m_delta_count;

		ReasonId m_reason;
		boost::uint64_t m_param1;
		boost::uint64_t m_param2;
		boost::uint64_t m_param3;

	public:
		ResourceTransactionElement(Operation operation, ResourceId resource_id, boost::uint64_t delta_count,
			ReasonId reason, boost::uint64_t param1, boost::uint64_t param2, boost::uint64_t param3)
			: m_operation(operation), m_resource_id(resource_id), m_delta_count(delta_count)
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
	Castle(MapObjectTypeId map_object_type_id, const AccountUuid &owner_uuid, std::string name);
	Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes);
	Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
		const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
		const std::vector<boost::shared_ptr<MySql::Center_CastleTech>> &techs,
		const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources);
	~Castle();

public:
	void pump_status(bool force_synchronization_with_client = false);
	void pump_building_status(BuildingBaseId building_base_id, bool force_synchronization_with_client = false);
	void pump_tech_status(TechId tech_id, bool force_synchronization_with_client = false);

	BuildingInfo get_building(BuildingBaseId building_base_id) const;
	void get_all_buildings(std::vector<BuildingInfo> &ret) const;
	std::size_t count_buildings_by_id(BuildingId building_id) const;
	void get_buildings_by_id(std::vector<BuildingInfo> &ret, BuildingId building_id) const;
	// 如果指定地基上有任务会抛出异常。
	void create_building_mission(BuildingBaseId building_base_id, Mission mission, BuildingId building_id = BuildingId());
	void cancel_building_mission(BuildingBaseId building_base_id);
	void speed_up_building_mission(BuildingBaseId building_base_id, boost::uint64_t delta_duration);

	TechInfo get_tech(TechId tech_id) const;
	void get_all_techs(std::vector<TechInfo> &ret) const;
	// 同上。
	void create_tech_mission(TechId tech_id, Mission mission);
	void cancel_tech_mission(TechId tech_id);
	void speed_up_tech_mission(TechId tech_id, boost::uint64_t delta_duration);

	ResourceInfo get_resource(ResourceId resource_id) const;
	void get_all_resources(std::vector<ResourceInfo> &ret) const;
	ResourceId commit_resource_transaction_nothrow(const ResourceTransactionElement *elements, std::size_t count,
		const boost::function<void ()> &callback = boost::function<void ()>());
	void commit_resource_transaction(const ResourceTransactionElement *elements, std::size_t count,
		const boost::function<void ()> &callback = boost::function<void ()>());
};

}

#endif
