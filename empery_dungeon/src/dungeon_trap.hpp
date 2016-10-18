#ifndef EMPERY_DUNGEON_DUNGEON_TRAP_HPP_
#define EMPERY_DUNGEON_DUNGEON_TRAP_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "data/dungeon_object.hpp"
#include "id_types.hpp"
#include "coord.hpp"
#include "dungeon.hpp"

namespace EmperyDungeon {
class DungeonClient;
class Dungeon;
class DungeonTrap : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const DungeonUuid m_dungeon_uuid;
	DungeonTrapTypeId m_dungeon_trap_type_id;
	Coord m_coord;
public:
	DungeonTrap(DungeonUuid dungeon_uuid,DungeonTrapTypeId trap_type_id,Coord coord);
	~DungeonTrap();
public:
	DungeonUuid get_dungeon_uuid() const {
		return m_dungeon_uuid;
	}
	DungeonTrapTypeId get_dungeon_trap_type_id() const {
		return m_dungeon_trap_type_id;
	}
	Coord get_coord() const {
		return m_coord;
	}
	void set_coord(Coord coord){
		m_coord = coord;
	}
};

}

#endif
