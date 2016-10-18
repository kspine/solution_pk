#include "precompiled.hpp"
#include "dungeon_trap.hpp"

namespace EmperyDungeon {

DungeonTrap::DungeonTrap(DungeonUuid dungeon_uuid,DungeonTrapTypeId trap_type_id,Coord coord)
	: m_dungeon_uuid(dungeon_uuid),m_dungeon_trap_type_id(trap_type_id), m_coord(coord)
{
}
DungeonTrap::~DungeonTrap(){
}
}
