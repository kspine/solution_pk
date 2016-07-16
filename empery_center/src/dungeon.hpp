#ifndef EMPERY_CENTER_DUNGEON_HPP_
#define EMPERY_CENTER_DUNGEON_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

class PlayerSession;
class DungeonSession;

class Dungeon : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const DungeonUuid m_dungeon_uuid;
	const DungeonTypeId m_dungeon_type_id;
	const std::uint64_t m_expiry_time;

	const boost::weak_ptr<DungeonSession> m_server;

	AccountUuid m_founder_uuid;
	boost::container::flat_set<AccountUuid> m_accounts;

public:
	Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id, std::uint64_t expiry_time,
		const boost::shared_ptr<DungeonSession> &server, AccountUuid founder_uuid);
	~Dungeon();

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

	bool is_account_in(AccountUuid account_uuid) const;
	void get_all_accounts(std::vector<AccountUuid> &ret) const;
	void insert_account(AccountUuid account_uuid);
	bool remove_account(AccountUuid account_uuid) noexcept;
	void clear() noexcept;

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
