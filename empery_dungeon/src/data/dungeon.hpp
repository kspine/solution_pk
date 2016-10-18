#ifndef EMPERY_DUNGEON_DATA_DUNGEON_HPP_
#define EMPERY_DUNGEON_DATA_DUNGEON_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyDungeon {

namespace Data {
	class Dungeon {
	public:
		static boost::shared_ptr<const Dungeon> get(DungeonTypeId dungeon_type_id);
		static boost::shared_ptr<const Dungeon> require(DungeonTypeId dungeon_type_id);

	public:
		DungeonTypeId dungeon_type_id;
		std::string   dungeon_map;
		std::string   dungeon_trigger;
	};

	class DungeonTrap {
	public:
		static boost::shared_ptr<const DungeonTrap> get(DungeonTrapTypeId dungeon_trap_type_id);
		static boost::shared_ptr<const DungeonTrap> require(DungeonTrapTypeId dungeon_trap_type_id);
	public:
		DungeonTrapTypeId trap_type_id;
		unsigned          attack_type;
		boost::uint64_t   attack_range;
		boost::uint64_t   attack;
	};
}

}

#endif
