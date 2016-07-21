#ifndef EMPERY_CENTER_DUNGEON_BOX_HPP_
#define EMPERY_CENTER_DUNGEON_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Dungeon;
}

class PlayerSession;

class DungeonBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum Score {
		S_NONE        = 0,
		S_PASSED      = 1,
		S_ONE_STAR    = 7,
		S_TWO_STARS   = 8,
		S_THREE_START = 9,
	};

	struct DungeonInfo {
		DungeonTypeId dungeon_type_id;
		Score score;
		std::uint64_t entry_count;
		std::uint64_t finish_count;
	};

private:
	const AccountUuid m_account_uuid;

	boost::container::flat_map<DungeonTypeId,
		boost::shared_ptr<MySql::Center_Dungeon>> m_dungeons;

public:
	DungeonBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_Dungeon>> &dungeons);
	~DungeonBox();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	DungeonInfo get(DungeonTypeId dungeon_type_id) const;
	void get_all(std::vector<DungeonInfo> &ret) const;
	void set(DungeonInfo info);
	bool remove(DungeonTypeId dungeon_type_id) noexcept;

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

}

#endif

