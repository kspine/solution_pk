#ifndef EMPERY_CENTER_DUNGEON_BUFF_HPP_
#define EMPERY_CENTER_DUNGEON_BUFF_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

class PlayerSession;
class DungeonSession;

class DungeonBuff : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const DungeonUuid m_dungeon_uuid;
	const DungeonBuffTypeId m_dungeon_buff_type_id;
	const DungeonObjectUuid m_create_uuid;
	const AccountUuid m_create_owner_uuid;

	Coord m_coord;
	std::uint64_t  m_expired_time;
	bool m_deleted = false;
public:
	DungeonBuff(DungeonUuid dungeon_uuid,DungeonBuffTypeId buff_type_id,DungeonObjectUuid create_uuid,AccountUuid create_owner_uuid,Coord coord,std::uint64_t expired_time);
	~DungeonBuff();

public:
	virtual void pump_status();

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

	std::uint64_t get_expired_time(){
		return m_expired_time;
	}

	void set_coord_no_synchronize(Coord coord) noexcept;

	bool has_been_deleted() const {
		return m_deleted;
	}
	void delete_from_game() noexcept;

	bool is_virtually_removed() const;
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
	void synchronize_with_dungeon_server(const boost::shared_ptr<DungeonSession> &server) const;
};


}

#endif
