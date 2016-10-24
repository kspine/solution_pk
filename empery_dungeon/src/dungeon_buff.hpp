#ifndef EMPERY_DUNGEON_DUNGEON_BUFF_HPP_
#define EMPERY_DUNGEON_DUNGEON_BUFF_HPP_

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

class DungeonBuff : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const DungeonUuid m_dungeon_uuid;
	DungeonBuffTypeId m_dungeon_buff_type_id;
	const DungeonObjectUuid m_create_uuid;
	const AccountUuid m_create_owner_uuid;
	Coord             m_coord;
	std::uint64_t     m_expired_time;
public:
	DungeonBuff(DungeonUuid dungeon_uuid,DungeonBuffTypeId buff_type_id,DungeonObjectUuid create_uuid,AccountUuid create_owner_uuid,Coord coord,std::uint64_t expired_time);
	~DungeonBuff();
public:
	DungeonUuid get_dungeon_uuid() const {
		return m_dungeon_uuid;
	}
	DungeonBuffTypeId get_dungeon_buff_type_id() const {
		return m_dungeon_buff_type_id;
	}
	
	DungeonObjectUuid get_create_uuid() const {
		return m_create_uuid;
	}

	AccountUuid  get_create_owner_uuid() const {
		return m_create_owner_uuid;
	}

	Coord get_coord() const {
		return m_coord;
	}
	void set_coord(Coord coord){
		m_coord = coord;
	}
	std::uint64_t   get_expired_time(){
		return m_expired_time;
	}
};

}

#endif
