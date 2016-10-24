#include "precompiled.hpp"
#include "dungeon_buff.hpp"

namespace EmperyDungeon {

DungeonBuff::DungeonBuff(DungeonUuid dungeon_uuid,DungeonBuffTypeId buff_type_id,DungeonObjectUuid create_uuid,AccountUuid create_owner_uuid,Coord coord,std::uint64_t expired_time)
	: m_dungeon_uuid(dungeon_uuid),m_dungeon_buff_type_id(buff_type_id),m_create_uuid(create_uuid),m_create_owner_uuid(create_owner_uuid),m_coord(coord),m_expired_time(expired_time)
{
}
DungeonBuff::~DungeonBuff(){
}


}
