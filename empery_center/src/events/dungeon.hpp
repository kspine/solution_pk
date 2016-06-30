#ifndef EMPERY_CENTER_EVENTS_DUNGEON_HPP_
#define EMPERY_CENTER_EVENTS_DUNGEON_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

namespace Events {
	struct DungeonCreated : public Poseidon::EventBase<340601> {
		AccountUuid account_uuid;
		DungeonId dungeon_id;
		unsigned old_score;

		DungeonCreated(AccountUuid account_uuid_, DungeonId dungeon_id_, unsigned old_score_)
			: account_uuid(account_uuid_), dungeon_id(dungeon_id_), old_score(old_score_)
		{
		}
	};

	struct DungeonDeleted : public Poseidon::EventBase<340602> {
		AccountUuid account_uuid;
		DungeonId dungeon_id;
		unsigned new_score;

		DungeonDeleted(AccountUuid account_uuid_, DungeonId dungeon_id_, unsigned new_score_)
			: account_uuid(account_uuid_), dungeon_id(dungeon_id_), new_score(new_score_)
		{
		}
	};
}

}

#endif
