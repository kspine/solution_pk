#ifndef EMPERY_CENTER_DUNGEON_HPP_
#define EMPERY_CENTER_DUNGEON_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

class DungeonObject;
class PlayerSession;
class DungeonSession;

class Dungeon : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum QuitReason {
		Q_DESTRUCTOR     = 0,
		Q_PLAYER_REQUEST = 1,
	};

private:
	const DungeonUuid m_dungeon_uuid;
	const DungeonTypeId m_dungeon_type_id;
	const std::uint64_t m_expiry_time;

	const boost::weak_ptr<DungeonSession> m_server;

	AccountUuid m_founder_uuid;
	boost::container::flat_map<AccountUuid, boost::weak_ptr<PlayerSession>> m_observers;
	boost::container::flat_map<DungeonObjectUuid, boost::shared_ptr<DungeonObject>> m_objects;

public:
	Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id, std::uint64_t expiry_time,
		const boost::shared_ptr<DungeonSession> &server, AccountUuid founder_uuid);
	~Dungeon();

private:
	void broadcast_to_observers(std::uint16_t message_id, const Poseidon::StreamBuffer &payload);
	template<class MessageT>
	void broadcast_to_observers(const MessageT &msg);

	void synchronize_with_all_observers(const boost::shared_ptr<DungeonObject> &dungeon_object) const noexcept;
	void synchronize_with_dungeon_server(const boost::shared_ptr<DungeonObject> &dungeon_object) const noexcept;

public:
	virtual void pump_status();

	DungeonUuid get_dungeon_uuid() const {
		return m_dungeon_uuid;
	}
	DungeonTypeId get_dungeon_type_id() const {
		return m_dungeon_type_id;
	}
	std::uint64_t get_expiry_time() const {
		return m_expiry_time;
	}

	AccountUuid get_founder_uuid() const {
		return m_founder_uuid;
	}
	void set_founder_uuid(AccountUuid founder_uuid);

	boost::shared_ptr<PlayerSession> get_observer(AccountUuid account_uuid) const;
	void get_observers_all(std::vector<std::pair<AccountUuid, boost::shared_ptr<PlayerSession>>> &ret) const;
	void insert_observer(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session);
	bool remove_observer(AccountUuid account_uuid, QuitReason reason, const char *param) noexcept;
	void clear_observers(QuitReason reason, const char *param) noexcept;

	boost::shared_ptr<DungeonObject> get_object(DungeonObjectUuid dungeon_object_uuid) const;
	void get_objects_all(std::vector<boost::shared_ptr<DungeonObject>> &ret) const;
	void insert_object(const boost::shared_ptr<DungeonObject> &dungeon_object);
	void update_object(const boost::shared_ptr<DungeonObject> &dungeon_object, bool throws_if_not_exists = true);

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

inline void synchronize_dungeon_with_player(const boost::shared_ptr<const Dungeon> &dungeon,
	const boost::shared_ptr<PlayerSession> &session)
{
	dungeon->synchronize_with_player(session);
}
inline void synchronize_dungeon_with_player(const boost::shared_ptr<Dungeon> &dungeon,
	const boost::shared_ptr<PlayerSession> &session)
{
	dungeon->synchronize_with_player(session);
}

}

#endif
