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

void LegionLog::LegionPersonalDonateTrace(AccountUuid account_uuid,std::uint64_t old_count,std::uint64_t new_count,ReasonId reason,std::uint64_t param1,std::uint64_t param2,std::uint64_t param3){
	boost::shared_ptr<Poseidon::EventBaseWithoutId> event = boost::make_shared<Events::LegionPersonalDonateChanged>(
					account_uuid,ItemId(1103005), old_count, new_count, reason, param1, param2, param3);
	Poseidon::async_raise_event(event);
}

void LegionLog::LegionMoneyTrace(LegionUuid legion_uuid,std::uint64_t old_count,std::uint64_t new_count,ReasonId reason,std::uint64_t param1,std::uint64_t param2,std::uint64_t param3){
	boost::shared_ptr<Poseidon::EventBaseWithoutId> event = boost::make_shared<Events::LegionMoneyChanged>(
					legion_uuid, ItemId(5500001), old_count, new_count, reason, param1, param2, param3);
	Poseidon::async_raise_event(event);
}

void LegionLog::CreateWarehouseBuildingTrace(AccountUuid account_uuid,LegionUuid legion_uuid,
				std::int64_t x,std::int64_t y,std::uint64_t created_time)
{
	const auto event = boost::make_shared<Events::CreateWarehouseBuildingTrace>(account_uuid, legion_uuid, x,y,created_time);

	const auto withdrawn = boost::make_shared<bool>(true);

	Poseidon::async_raise_event(event, withdrawn);

	*withdrawn = false;
}

void LegionLog::OpenWarehouseBuildingTrace(AccountUuid account_uuid,LegionUuid legion_uuid,
				std::int64_t x,std::int64_t y,std::uint64_t level,std::uint64_t open_time)
{
	const auto event = boost::make_shared<Events::OpenWarehouseBuildingTrace>(account_uuid, legion_uuid, x,y,level,open_time);

	const auto withdrawn = boost::make_shared<bool>(true);

	Poseidon::async_raise_event(event, withdrawn);

	*withdrawn = false;
}
void LegionLog::RobWarehouseBuildingTrace(AccountUuid account_uuid,LegionUuid legion_uuid,
				std::uint64_t level,std::uint64_t ntype,std::uint64_t amount,std::uint64_t rob_time)
{
	const auto event = boost::make_shared<Events::RobWarehouseBuildingTrace>(account_uuid, legion_uuid, level,ntype,amount,rob_time);

	const auto withdrawn = boost::make_shared<bool>(true);

	Poseidon::async_raise_event(event, withdrawn);

	*withdrawn = false;
}

}
