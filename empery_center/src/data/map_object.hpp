#ifndef EMPERY_CENTER_DATA_MAP_OBJECT_HPP_
#define EMPERY_CENTER_DATA_MAP_OBJECT_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCenter {

namespace Data {
	class MapObjectType {
	public:
		static boost::shared_ptr<const MapObjectType> get(MapObjectTypeId map_object_type_id);
		static boost::shared_ptr<const MapObjectType> require(MapObjectTypeId map_object_type_id);

	public:
		MapObjectTypeId map_object_type_id;
		double speed;
		double harvest_speed;

		boost::container::flat_map<BuildingId, unsigned> prerequisite;
		boost::container::flat_map<ResourceId, std::uint64_t> enability_cost;

		boost::container::flat_map<ResourceId, std::uint64_t> production_cost;
		boost::container::flat_map<ResourceId, std::uint64_t> maintenance_cost;
		MapObjectTypeId previous_id;
		double production_time;
		BuildingId factory_id;
		std::uint64_t max_rally_count;
	};
}

}

#endif
