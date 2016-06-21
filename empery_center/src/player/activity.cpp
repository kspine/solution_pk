#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/cs_activity.hpp"
#include "../msg/sc_activity.hpp"
#include "../msg/err_activity.hpp"
#include "../singletons/activity_map.hpp"
#include "../singletons/map_activity_accumulate_map.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/world_map.hpp"
#include "../activity.hpp"
#include "../activity_ids.hpp"
#include "../castle.hpp"


namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_MapActivityInfo, account, session, /* req */){
	const auto map_activity = ActivityMap::get_map_activity();
	if(!map_activity){
		return Response(Msg::ERR_NO_MAP_ACTIVITY);
	}
	map_activity->synchronize_with_player(session);
	return Response();
}

PLAYER_SERVLET(Msg::CS_MapActivityKillSolidersRank, account, session, /* req */){
	const auto map_activity = ActivityMap::get_map_activity();
	if(!map_activity){
		return Response(Msg::ERR_NO_MAP_ACTIVITY);
	}
	MapActivity::MapActivityDetailInfo activity_kill_solidier_info = map_activity->get_activity_info(MapActivityId(ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER));
	if(activity_kill_solidier_info.unique_id != ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER.get()){
		return Response(Msg::ERR_NO_MAP_ACTIVITY);
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now <= activity_kill_solidier_info.available_until){
		return Response(Msg::ERR_NO_ACTIVITY_FINISH);
	}
	std::vector<MapActivityRankMap::MapActivityRankInfo> ret;
	MapActivityRankMap::get_recent_rank_list(MapActivityId(ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER),activity_kill_solidier_info.available_until,ret);
	if(ret.empty()){
		map_activity->settle_kill_soliders_activity(utc_now);
		MapActivityRankMap::get_recent_rank_list(MapActivityId(ActivityIds::ID_MAP_ACTIVITY_KILL_SOLDIER),activity_kill_solidier_info.available_until,ret);
	}
	if(session){
		try {
			Msg::SC_MapActivityKillSolidersRank msgRankList;
			for(auto it = ret.begin(); it != ret.end(); ++it){
				auto &rank_item = *msgRankList.rank_list.emplace(msgRankList.rank_list.end());
				const auto &account = AccountMap::require(it->account_uuid);
				const auto &castle = WorldMap::require_primary_castle(it->account_uuid);
				rank_item.account_uuid      = it->account_uuid.str();
				rank_item.nick              = account->get_nick();
				rank_item.castle_name       = castle->get_name();
				rank_item.leagues           = "";
				rank_item.rank              = it->rank;
				rank_item.accumulate_value  = it->accumulate_value;
			}
			session->send(msgRankList);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
	return Response();
}

}
