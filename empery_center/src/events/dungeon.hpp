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

		DungeonDeleted(AccountUuid account_uuid_, DungeonTypeId dungeon_type_id_)
			: account_uuid(account_uuid_), dungeon_type_id(dungeon_type_id_)
		{
		}
	};

	struct DungeonFinish : public Poseidon::EventBase<340603> {
		AccountUuid account_uuid;
		DungeonTypeId dungeon_type_id;
		std::uint64_t begin_time;
		std::uint64_t finish_time;
		bool finished;

		DungeonFinish(AccountUuid account_uuid_, DungeonTypeId dungeon_type_id_, std::uint64_t begin_time_,std::uint64_t finish_time_, bool finished_)
			: account_uuid(account_uuid_), dungeon_type_id(dungeon_type_id_), begin_time(begin_time_),finish_time(finish_time_), finished(finished_)
		{
		}
	};
}

}

#endif
