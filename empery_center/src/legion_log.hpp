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

	enum LEAGUE_LEGION_LOG
	{
		ELEAGUE_CREATE_JOIN = 0,
		ELEAGUE_APPLY_JOIN = 1,
		ELEAGUE_INVITE_JOIN = 2,
		ELEAGUE_CHECK_JOIN = 3,
		ELEAGUE_EXIT = 4,
		ELEAGUE_KICK = 5
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

	static void CreateWarehouseBuildingTrace(AccountUuid account_uuid,LegionUuid legion_uuid,
				std::int64_t x,std::int64_t y,std::uint64_t created_time);

	static void OpenWarehouseBuildingTrace(AccountUuid account_uuid,LegionUuid legion_uuid,
				std::int64_t x,std::int64_t y,std::uint64_t level,std::uint64_t open_time);
	
	static void RobWarehouseBuildingTrace(AccountUuid account_uuid,LegionUuid legion_uuid,
				std::uint64_t level,std::uint64_t ntype,std::uint64_t amount,std::uint64_t rob_time);

};

}

#endif
