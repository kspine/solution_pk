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

	struct LegionPersonalDonateChanged : public Poseidon::EventBase<340720> {
		AccountUuid   account_uuid;
		ItemId        item_id;
		std::uint64_t old_count;
		std::uint64_t new_count;

		ReasonId reason;
		std::int64_t param1;
		std::int64_t param2;
		std::int64_t param3;

		LegionPersonalDonateChanged(AccountUuid account_uuid_,
			ItemId item_id_, std::uint64_t old_count_, std::uint64_t new_count_,
			ReasonId reason_, std::int64_t param1_, std::int64_t param2_, std::int64_t param3_)
			: account_uuid(account_uuid_)
			, item_id(item_id_), old_count(old_count_), new_count(new_count_)
			, reason(reason_), param1(param1_), param2(param2_), param3(param3_)
		{
		}
	};

	struct LegionMoneyChanged : public Poseidon::EventBase<340721> {
		LegionUuid    legion_uuid;
		ItemId        item_id;
		std::uint64_t old_count;
		std::uint64_t new_count;

		ReasonId reason;
		std::int64_t param1;
		std::int64_t param2;
		std::int64_t param3;

		LegionMoneyChanged(LegionUuid legion_uuid_,
			ItemId item_id_, std::uint64_t old_count_, std::uint64_t new_count_,
			ReasonId reason_, std::int64_t param1_, std::int64_t param2_, std::int64_t param3_)
			: legion_uuid(legion_uuid_)
			, item_id(item_id_), old_count(old_count_), new_count(new_count_)
			, reason(reason_), param1(param1_), param2(param2_), param3(param3_)
		{
		}
	};
}

}

#endif
