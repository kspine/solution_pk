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

class DungeonPassPoint : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
typedef std::pair<std::string,unsigned> MONSTER_STATE;
typedef std::pair<unsigned,std::vector<MONSTER_STATE>> BLOCK_STATE;
private:
	const DungeonUuid m_dungeon_uuid;
	Coord m_coord;
	bool m_deleted = false;
	bool m_state   = false;
	boost::container::flat_map<Coord,BLOCK_STATE> m_blocks;
public:
	DungeonPassPoint(DungeonUuid dungeon_uuid,Coord coord,boost::container::flat_map<Coord,BLOCK_STATE> blocks);
	~DungeonPassPoint();
public:
	DungeonUuid get_dungeon_uuid() const {
		return m_dungeon_uuid;
	}

	Coord get_coord() const {
		return m_coord;
	}
	void set_state(bool state){
		m_state = state;
	}
	bool get_state(){
		return m_state;
	}
};

}

#endif
