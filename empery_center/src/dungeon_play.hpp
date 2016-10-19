#ifndef EMPERY_CENTER_DUNGEON_PLAY_HPP_
#define EMPERY_CENTER_DUNGEON_PLAY_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

class PlayerSession;
class DungeonSession;

class DungeonTrap : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const DungeonUuid m_dungeon_uuid;
	const DungeonTrapTypeId m_dungeon_trap_type_id;

	Coord m_coord;
	bool m_deleted = false;
public:
	DungeonTrap(DungeonUuid dungeon_uuid,DungeonTrapTypeId trap_type_id,Coord coord);
	~DungeonTrap();

public:
	virtual void pump_status();

	DungeonUuid get_dungeon_uuid() const {
		return m_dungeon_uuid;
	}
	
	DungeonTrapTypeId get_dungeon_trap_type_id() const {
		return m_dungeon_trap_type_id;
	}

	Coord get_coord() const {
		return m_coord;
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
	virtual void pump_status();

	DungeonUuid get_dungeon_uuid() const {
		return m_dungeon_uuid;
	}

	Coord get_coord() const {
		return m_coord;
	}
	void set_coord_no_synchronize(Coord coord) noexcept;

	bool has_been_deleted() const {
		return m_deleted;
	}
	void delete_from_game() noexcept;
	bool is_block_coord(Coord coord);
	bool update_block_monster(std::string tag);
	bool is_virtually_removed() const;
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
	void synchronize_with_dungeon_server(const boost::shared_ptr<DungeonSession> &server) const;
};

}

#endif
