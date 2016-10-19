#include "precompiled.hpp"
#include "dungeon_play.hpp"

namespace EmperyDungeon {

DungeonTrap::DungeonTrap(DungeonUuid dungeon_uuid,DungeonTrapTypeId trap_type_id,Coord coord)
	: m_dungeon_uuid(dungeon_uuid),m_dungeon_trap_type_id(trap_type_id), m_coord(coord)
{
}
DungeonTrap::~DungeonTrap(){
}

DungeonPassPoint::DungeonPassPoint(DungeonUuid dungeon_uuid,Coord coord,boost::container::flat_map<Coord,BLOCK_STATE> blocks)
	: m_dungeon_uuid(dungeon_uuid), m_coord(coord),m_blocks(std::move(blocks))
{
}
DungeonPassPoint::~DungeonPassPoint(){
}
}
