#ifndef EMPERY_CENTER_EVENTS_LEGION_HPP_
#define EMPERY_CENTER_EVENTS_LEGION_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

namespace Events {
	struct LegionDisband : public Poseidon::EventBase<340701> {
		AccountUuid account_uuid;
		std::string legion_name;
		std::uint64_t disband_time;

		LegionDisband(AccountUuid account_uuid_, std::string legion_name_,std::uint64_t disband_time_)
			: account_uuid(account_uuid_), legion_name(legion_name_),disband_time(disband_time_)
		{
		}
	};

	struct LeagueDisband : public Poseidon::EventBase<340702> {
		AccountUuid account_uuid;
		std::string league_name;
		std::uint64_t disband_time;

		LeagueDisband(AccountUuid account_uuid_, std::string league_name_,std::uint64_t disband_time_)
			: account_uuid(account_uuid_), league_name(league_name_),disband_time(disband_time_)
		{
		}
	};
	
}

}

#endif
