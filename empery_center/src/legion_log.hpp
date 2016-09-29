#ifndef EMPERY_CENTER_LEGIONLOG_HPP_
#define EMPERY_CENTER_LEGIONLOG_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/fwd.hpp>

#include "id_types.hpp"

namespace EmperyCenter {

class LegionLog {
public:
	enum LEGION_MEMBER_LOG
	{
		 ELEGION_APPLY_JOIN  = 0,
		 ELEGION_INVITE_JOIN = 1,
		 ELEGION_CHECK_JOIN  = 2,
		 ELEGION_EXIT        = 3,
		 ELEGION_KICK        = 4
	};

public:
	LegionLog();
	~LegionLog();
public:
	static void LegionMemberTrace(AccountUuid account_uuid,
			LegionUuid legion_uuid,AccountUuid action_uuid,
			std::uint64_t action_type,std::uint64_t created_time);

	static void LeagueLegionTrace(LegionUuid legion_uuid,
			LeagueUuid league_uuid,LegionUuid action_uuid,
			std::uint64_t action_type,std::uint64_t created_time);

	static void LegionDisbandTrace(AccountUuid account_uuid,std::string legion_name,std::uint64_t disband_time);

	static void LeagueDisbandTrace(AccountUuid account_uuid,std::string league_name,std::uint64_t disband_time);
	static void LegionMoneyTrace(LegionUuid legion_uuid,std::uint64_t old_count,std::uint64_t new_count,ReasonId reason,std::uint64_t param1,std::uint64_t param2,std::uint64_t param3);
	static void LegionPersonalDonateTrace(AccountUuid account_uuid,std::uint64_t old_count,std::uint64_t new_count,ReasonId reason,std::uint64_t param1,std::uint64_t param2,std::uint64_t param3);
	
};

}

#endif
