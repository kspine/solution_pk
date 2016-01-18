#ifndef EMPERY_CENTER_DATA_MAP_OBJECT_HPP_
#define EMPERY_CENTER_DATA_MAP_OBJECT_HPP_

#include "common.hpp"

namespace EmperyCenter {

namespace Data {
	class MapObject {
	public:
		static boost::shared_ptr<const MapObject> get(MapObjectTypeId map_object_type_id);
		static boost::shared_ptr<const MapObject> require(MapObjectTypeId map_object_type_id);

	public:
		MapObjectTypeId map_object_type_id;
		double speed;
		double harvest_speed;
	};
}

}

#endif
