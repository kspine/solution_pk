#ifndef EMPERY_CENTER_DATA_MAP_OBJECT_HPP_
#define EMPERY_CENTER_DATA_MAP_OBJECT_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCenter {

namespace Data {
	class MapObjectTypeAbstract {
	public:
		static boost::shared_ptr<const MapObjectTypeAbstract> get(MapObjectTypeId map_object_type_id);
		static boost::shared_ptr<const MapObjectTypeAbstract> require(MapObjectTypeId map_object_type_id);

	public:
		MapObjectTypeId map_object_type_id;
		MapObjectCategoryId map_object_category_id;

		std::uint64_t max_soldier_count;
		double speed;
	};

	class MapObjectTypeBattalion : public MapObjectTypeAbstract {
	public:
		static boost::shared_ptr<const MapObjectTypeBattalion> get(MapObjectTypeId map_object_type_id);
		static boost::shared_ptr<const MapObjectTypeBattalion> require(MapObjectTypeId map_object_type_id);

	public:
		double harvest_speed;

		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		boost::container::flat_map<ResourceId, std::uint64_t> enability_cost;

		boost::container::flat_map<ResourceId, std::uint64_t> production_cost;
		boost::container::flat_map<ResourceId, std::uint64_t> maintenance_cost;

		MapObjectTypeId previous_id;
		double production_time;
		BuildingId factory_id;
	};

	class MapObjectTypeMonster : public MapObjectTypeAbstract {
	public:
		static boost::shared_ptr<const MapObjectTypeMonster> get(MapObjectTypeId map_object_type_id);
		static boost::shared_ptr<const MapObjectTypeMonster> require(MapObjectTypeId map_object_type_id);

	public:
		std::uint64_t return_range;

		boost::container::flat_map<SharedNts, std::uint64_t> reward_set;
	};
}

}

#endif
