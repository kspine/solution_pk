#ifndef EMPERY_CENTER_DUNGEON_OBJECT_HPP_
#define EMPERY_CENTER_DUNGEON_OBJECT_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

class PlayerSession;
class DungeonSession;

class DungeonObject : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	struct BuffInfo {
		BuffId buff_id;
		std::uint64_t duration;
		std::uint64_t time_begin;
		std::uint64_t time_end;
	};

private:
	const DungeonUuid m_dungeon_uuid;
	const DungeonObjectUuid m_dungeon_object_uuid;
	const MapObjectTypeId m_map_object_type_id;
	const AccountUuid m_owner_uuid;
	const std::string m_tag;

	Coord m_coord;
	bool m_deleted = false;

	boost::container::flat_map<AttributeId, std::int64_t> m_attributes;
	boost::container::flat_map<BuffId, BuffInfo> m_buffs;

	unsigned m_action = 0;
	std::string m_action_param;

public:
	DungeonObject(DungeonUuid dungeon_uuid, DungeonObjectUuid dungeon_object_uuid,
		MapObjectTypeId map_object_type_id, AccountUuid owner_uuid, std::string tag, Coord coord);
	~DungeonObject();

public:
	virtual void pump_status();
	virtual void recalculate_attributes(bool recursive);

	DungeonUuid get_dungeon_uuid() const {
		return m_dungeon_uuid;
	}
	DungeonObjectUuid get_dungeon_object_uuid() const {
		return m_dungeon_object_uuid;
	}
	MapObjectTypeId get_map_object_type_id() const {
		return m_map_object_type_id;
	}
	AccountUuid get_owner_uuid() const {
		return m_owner_uuid;
	}
	const std::string &get_tag() const {
		return m_tag;
	}

	Coord get_coord() const {
		return m_coord;
	}
	void set_coord_no_synchronize(Coord coord) noexcept;

	bool has_been_deleted() const {
		return m_deleted;
	}
	void delete_from_game() noexcept;

	std::int64_t get_attribute(AttributeId attribute_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const;
	void set_attributes(boost::container::flat_map<AttributeId, std::int64_t> modifiers);

	BuffInfo get_buff(BuffId buff_id) const;
	bool is_buff_in_effect(BuffId buff_id) const;
	void get_buffs(std::vector<BuffInfo> &ret) const;
	void set_buff(BuffId buff_id, std::uint64_t duration);
	void set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration);
	void accumulate_buff(BuffId buff_id, std::uint64_t delta_duration);
	void clear_buff(BuffId buff_id) noexcept;

	unsigned get_action() const {
		return m_action;
	}
	const std::string &get_action_param() const {
		return m_action_param;
	}
	void set_action(unsigned action, std::string action_param);

	bool is_virtually_removed() const;
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
	void synchronize_with_dungeon_server(const boost::shared_ptr<DungeonSession> &server) const;
};

}

#endif
