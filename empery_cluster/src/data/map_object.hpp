#ifndef EMPERY_CENTER_DATA_MAP_OBJECT_HPP_
#define EMPERY_CENTER_DATA_MAP_OBJECT_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCluster {

namespace Data {
	class MapObjectType {
	public:
		static boost::shared_ptr<const MapObjectType> get(MapObjectTypeId map_object_type_id);
		static boost::shared_ptr<const MapObjectType> require(MapObjectTypeId map_object_type_id);
	public:
		MapObjectTypeId     map_object_type_id;
		double              attack;
		double              defence;
		unsigned            shoot_range;
		double              attack_speed;
		double              first_attack;
	};
}

}

#endif
