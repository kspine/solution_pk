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
		MapObjectWeaponId   category_id;
		MapObjectChassisId map_object_chassis_id;
		double              attack;
		double              defence;
		double              speed;
		unsigned            shoot_range;
		double              attack_speed;
		double              attack_plus;
		double              harvest_speed;
		double              doge_rate;
		double              critical_rate;
		double              critical_damage_plus_rate;
		unsigned            attack_type;
		unsigned            defence_type;
		std::uint32_t       hp;
		std::uint32_t       ai_id;
		std::uint32_t       view;
	};

	class MapObjectRelative {
		public:
		enum AttackType : unsigned {
			ATTACK_NORMAL = 1,
			ATTACK_STAB   = 2,
			ATTACK_CITY   = 3,
			ATTACK_FLAME  = 4,
		};

		enum DefenceType: unsigned {
			DEFENCE_LIGHT         = 1,
			DEFENCE_MIDDLE        = 2,
			DEFENCE_HEAVY         = 3,
			DEFENCE_REINFORCE	  = 4,
		};
	public:
		static double get_relative(unsigned attack_type,unsigned defence_type);
	public:
		unsigned 	attack_type;
		boost::container::flat_map<unsigned, double> arm_relative;
	};

	class MapObjectTypeMonster{
	public:
		static boost::shared_ptr<const MapObjectTypeMonster> get(MapObjectTypeId map_object_type_id);
	public:
		MapObjectTypeId     map_object_type_id;
		MapObjectWeaponId   arm_relative_id;
		std::int64_t       level;
	};

	class MapObjectTypeBuilding{
	public:
		static boost::shared_ptr<const MapObjectTypeBuilding> get(MapObjectTypeId map_object_type_id,std::uint32_t level);
	public:
		MapObjectTypeId    map_object_type_id;
		std::uint32_t      level;
		MapObjectTypeId    arm_type_id;
		std::pair<MapObjectTypeId,std::uint32_t> type_level;
	};

	class MapObjectAi{
	public:
		static boost::shared_ptr<const MapObjectAi> get(std::uint64_t unique_id);
	public:
		std::uint64_t unique_id;
		unsigned      ai_type;
		std::string   params;
	};
}

}

#endif
