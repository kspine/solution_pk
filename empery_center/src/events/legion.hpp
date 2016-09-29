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

	struct CreateWarehouseBuildingTrace : public Poseidon::EventBase<340703> {
		AccountUuid account_uuid;
		LegionUuid  legion_uuid;
		std::int64_t x;
		std::int64_t y;
		std::uint64_t created_time;

		CreateWarehouseBuildingTrace(AccountUuid account_uuid_, LegionUuid legion_uuid_,std::int64_t x_,std::int64_t y_,std::uint64_t created_time_)
			: account_uuid(account_uuid_), legion_uuid(legion_uuid_),x(x_),y(y_),created_time(created_time_)
		{
		}
	};

	struct OpenWarehouseBuildingTrace : public Poseidon::EventBase<340704> {
		AccountUuid account_uuid;
		LegionUuid  legion_uuid;
		std::int64_t x;
		std::int64_t y;
		std::uint64_t level;
		std::uint64_t open_time;

		OpenWarehouseBuildingTrace(AccountUuid account_uuid_, LegionUuid legion_uuid_,std::int64_t x_,std::int64_t y_,std::uint64_t level_,std::uint64_t open_time_)
			: account_uuid(account_uuid_), legion_uuid(legion_uuid_),x(x_),y(y_),level(level_),open_time(open_time_)
		{
		}
	};

	struct RobWarehouseBuildingTrace : public Poseidon::EventBase<340705> {
		AccountUuid account_uuid;
		LegionUuid  legion_uuid;
		std::uint64_t level;
		std::uint64_t ntype;
		std::uint64_t amount;
		std::uint64_t rob_time;


		RobWarehouseBuildingTrace(AccountUuid account_uuid_, LegionUuid legion_uuid_,std::uint64_t level_,std::uint64_t ntype_,std::uint64_t amount_,std::uint64_t rob_time_)
			: account_uuid(account_uuid_), legion_uuid(legion_uuid_),level(level_),ntype(ntype_),amount(amount_),rob_time(rob_time_)
		{
		}
	};
	

	struct LegionMemberTrace : public Poseidon::EventBase<340710> {
		AccountUuid account_uuid;
		LegionUuid legion_uuid;
		AccountUuid action_uuid;
		std::uint64_t action_type;
		std::uint64_t created_time;

		LegionMemberTrace(
				AccountUuid account_uuid_,
				LegionUuid legion_uuid_,
				AccountUuid action_uuid_,
				std::uint64_t action_type_,
				std::uint64_t created_time_)
		:account_uuid(account_uuid_),
			legion_uuid(legion_uuid_),
			action_uuid(action_uuid_),
			action_type(action_type_),
			created_time(created_time_)
		{
		}
	};


	struct LeagueLegionTrace : public Poseidon::EventBase<340711> {
		LegionUuid legion_uuid;
		LeagueUuid league_uuid;
		LegionUuid action_uuid;
		std::uint64_t action_type;
		std::uint64_t created_time;

		LeagueLegionTrace(
				LegionUuid legion_uuid_,
				LeagueUuid league_uuid_,
				LegionUuid action_uuid_,
				std::uint64_t action_type_,
				std::uint64_t created_time_)
		:legion_uuid(legion_uuid_),
		 league_uuid(league_uuid_),
			action_uuid(action_uuid_),
			action_type(action_type_),
			created_time(created_time_)
		{
		}
	};
}

}

#endif
