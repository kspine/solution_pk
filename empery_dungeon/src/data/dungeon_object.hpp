#ifndef EMPERY_DUNGEON_DATA_DUNGEON_OBJECT_HPP_
#define EMPERY_DUNGEON_DATA_DUNGEON_OBJECT_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyDungeon {

namespace Data {
	class DungeonObjectType {
	public:
		static boost::shared_ptr<const DungeonObjectType> get(DungeonObjectTypeId dungeon_object_type_id);
		static boost::shared_ptr<const DungeonObjectType> require(DungeonObjectTypeId dungeon_object_type_id);
	public:
		DungeonObjectTypeId  dungeon_object_type_id;
		DungeonObjectWeaponId   category_id;
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
		std::uint64_t       max_soldier_count;
		std::uint64_t       hp;
		std::uint32_t       ai_id;
		std::uint32_t       view;
		std::vector<std::pair<std::uint32_t,std::uint32_t>> skill_trigger;
		std::vector<std::vector<DungeonMonsterSkillId>> skills;
	};

	class DungeonObjectRelative {
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

	class DungeonObjectTypeMonster{
	public:
		static boost::shared_ptr<const DungeonObjectTypeMonster> get(DungeonObjectTypeId dungeon_object_type_id);
	public:
		DungeonObjectTypeId     dungeon_object_type_id;
		DungeonObjectWeaponId   arm_relative_id;
	};

	class DungeonObjectAi{
	public:
		static boost::shared_ptr<const DungeonObjectAi> get(std::uint64_t unique_id);
		static boost::shared_ptr<const DungeonObjectAi> require(std::uint64_t unique_id);
	public:
		std::uint64_t unique_id;
		unsigned      ai_type;
		std::string   params;
		std::string   params2;
		unsigned      ai_linkage;
		unsigned      ai_Intelligence;
	};
}

}

#endif
