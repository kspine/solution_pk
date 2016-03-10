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
		MapObjectWeaponId category_id;
		double              attack;
		double              defence;
		double              speed;
		unsigned            shoot_range;
		double              attack_speed;
		double              attack_plus;
		double              doge_rate;
		double              critical_rate;
		double              critical_damage_plus_rate;
	};
	
	class MapObjectRelative {
	public:
		static double get_relative(MapObjectWeaponId map_object_category_id,MapObjectWeaponId relateive_category_id);
	public:
		MapObjectWeaponId           category_id;
		boost::container::flat_map<MapObjectWeaponId, double> arm_relative;
	};
}

}

#endif
