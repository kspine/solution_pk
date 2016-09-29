#include "precompiled.hpp"
#include "mmain.hpp"
#include "legion_log.hpp"
#include "msg/sl_legion.hpp"
#include "events/legion.hpp"


namespace EmperyCenter {


LegionLog::LegionLog()
{
}
LegionLog::~LegionLog(){
}


void LegionLog::LegionMemberTrace(AccountUuid account_uuid,
			LegionUuid legion_uuid,AccountUuid action_uuid,
			std::uint64_t action_type,std::uint64_t created_time)
{
	const auto event = boost::make_shared<Events::LegionMemberTrace>(
			account_uuid,legion_uuid,action_uuid,action_type,created_time);

	const auto withdrawn = boost::make_shared<bool>(true);

	Poseidon::async_raise_event(event, withdrawn);

	*withdrawn = false;
}

void LegionLog::LeagueLegionTrace(LegionUuid legion_uuid,
			LeagueUuid league_uuid,LegionUuid action_uuid,
			std::uint64_t action_type,std::uint64_t created_time)
{
	const auto event = boost::make_shared<Events::LeagueLegionTrace>(
			legion_uuid,league_uuid,action_uuid,action_type,created_time);

		const auto withdrawn = boost::make_shared<bool>(true);

		Poseidon::async_raise_event(event, withdrawn);

		*withdrawn = false;
}

void LegionLog::LegionDisbandTrace(AccountUuid account_uuid,std::string legion_name,std::uint64_t disband_time)
{
	const auto event = boost::make_shared<Events::LegionDisband>(account_uuid, legion_name, disband_time);

	const auto withdrawn = boost::make_shared<bool>(true);

	Poseidon::async_raise_event(event, withdrawn);

	*withdrawn = false;
}

void LegionLog::LeagueDisbandTrace(AccountUuid account_uuid,std::string league_name,std::uint64_t disband_time)
{
	const auto event = boost::make_shared<Events::LeagueDisband>(account_uuid, league_name, disband_time);

	const auto withdrawn = boost::make_shared<bool>(true);

	Poseidon::async_raise_event(event, withdrawn);

	*withdrawn = false;
}

}
