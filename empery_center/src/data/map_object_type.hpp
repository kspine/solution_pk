#ifndef EMPERY_CENTER_DATA_MAP_OBJECT_TYPE_HPP_
#define EMPERY_CENTER_DATA_MAP_OBJECT_TYPE_HPP_

#include "common.hpp"

namespace EmperyCenter {

namespace Data {
	class MapObjectType {
	public:
		static boost::shared_ptr<const MapObjectType> get(MapObjectTypeId map_object_type_id);
		static boost::shared_ptr<const MapObjectType> require(MapObjectTypeId map_object_type_id);

	public:
		MapObjectTypeId map_object_type_id;
		boost::uint64_t ms_per_cell; // 移动速度，走每单位格子需要的毫秒数。为零不可移动。
	};
}

}

#endif
