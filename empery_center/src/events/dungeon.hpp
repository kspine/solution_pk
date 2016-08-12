#ifndef EMPERY_CENTER_EVENTS_DUNGEON_HPP_
#define EMPERY_CENTER_EVENTS_DUNGEON_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

namespace Events {
	struct DungeonCreated : public Poseidon::EventBase<340601> {
		AccountUuid account_uuid;
		DungeonTypeId dungeon_type_id;

		DungeonCreated(AccountUuid account_uuid_, DungeonTypeId dungeon_type_id_)
			: account_uuid(account_uuid_), dungeon_type_id(dungeon_type_id_)
		{
		}
	};

	struct DungeonDeleted : public Poseidon::EventBase<340602> {
		AccountUuid account_uuid;
		DungeonTypeId dungeon_type_id;
		bool finished;

		DungeonDeleted(AccountUuid account_uuid_, DungeonTypeId dungeon_type_id_, bool finished_)
			: account_uuid(account_uuid_), dungeon_type_id(dungeon_type_id_), finished(finished_)
		{
		}
	};
}

}

#endif
