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
#include "../data/activity.hpp"


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
	if(session){
		try {
			map_activity->synchronize_kill_soliders_rank_with_player(session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}
	return Response();
}

PLAYER_SERVLET(Msg::CS_WorldActivityInfo, account, session, /* req */){
	const auto world_activity = ActivityMap::get_world_activity();
	if(!world_activity){
		return Response(Msg::ERR_NO_WORLD_ACTIVITY);
	}
	try {
		const auto account_uuid = account->get_account_uuid();
		const auto &castle = WorldMap::require_primary_castle(account_uuid);
		const auto cluster_coord = WorldMap::get_cluster_scope(castle->get_coord()).bottom_left();
		world_activity->synchronize_with_player(cluster_coord,account_uuid,session);
	} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}
	return Response();
}

PLAYER_SERVLET(Msg::CS_WorldActivityRank, account, session, /* req */){
	const auto world_activity = ActivityMap::get_world_activity();
	if(!world_activity){
		return Response(Msg::ERR_NO_WORLD_ACTIVITY);
	}
	try {
		const auto account_uuid = account->get_account_uuid();
		const auto &castle = WorldMap::require_primary_castle(account_uuid);
		const auto cluster_coord = WorldMap::get_cluster_scope(castle->get_coord()).bottom_left();
		world_activity->synchronize_world_rank_with_player(cluster_coord,account_uuid,session);
	} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}

	return Response();
}


PLAYER_SERVLET(Msg::CS_WorldBossPos, account, session, /* req */){
	const auto world_activity = ActivityMap::get_world_activity();
	if(!world_activity){
		return Response(Msg::ERR_NO_WORLD_ACTIVITY);
	}
	try {
		const auto account_uuid = account->get_account_uuid();
		const auto &castle = WorldMap::require_primary_castle(account_uuid);
		const auto cluster_coord = WorldMap::get_cluster_scope(castle->get_coord()).bottom_left();
		world_activity->synchronize_world_boss_with_player(cluster_coord,session);
	} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_ClaimWorldActivity, account, session, req){
	const auto world_activity = ActivityMap::get_world_activity();
	if(!world_activity){
		return Response(Msg::ERR_NO_WORLD_ACTIVITY);
	}
	const auto sub_world_activity_id = WorldActivityId(req.unique_id);
	const auto account_uuid = account->get_account_uuid();
	const auto &castle = WorldMap::require_primary_castle(account_uuid);
	const auto cluster_coord = WorldMap::get_cluster_scope(castle->get_coord()).bottom_left();
	WorldActivityMap::WorldActivityInfo world_activity_info = WorldActivityMap::get(cluster_coord,sub_world_activity_id,world_activity->m_available_since);
	if(world_activity_info.activity_id != sub_world_activity_id){
		return Response(Msg::ERR_NO_WORLD_ACTIVITY);
	}
	if(!world_activity_info.finish){
		return Response(Msg::ERR_NO_ACTIVITY_FINISH);
	}
	WorldActivityAccumulateMap::WorldActivityAccumulateInfo account_accumulate_info = WorldActivityAccumulateMap::get(account_uuid,cluster_coord, sub_world_activity_id,world_activity->m_available_since);
	if(account_accumulate_info.rewarded){
		return Response(Msg::ERR_WORLD_ACTIVITY_HAVE_REWARDED)<<sub_world_activity_id;
	}
	world_activity->reward_activity(cluster_coord,sub_world_activity_id,account_uuid);
	return Response();
}

PLAYER_SERVLET(Msg::CS_ClaimWorldActivityRank, account, session, req){
	const auto world_activity = ActivityMap::get_world_activity();
	if(!world_activity){
		return Response(Msg::ERR_NO_WORLD_ACTIVITY);
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < world_activity->m_available_until){
		return Response(Msg::ERR_NO_ACTIVITY_FINISH);
	}
	const auto account_uuid = account->get_account_uuid();
	const auto &castle = WorldMap::require_primary_castle(account_uuid);
	const auto cluster_coord = WorldMap::get_cluster_scope(castle->get_coord()).bottom_left();
	WorldActivityRankMap::WorldActivityRankInfo self_info = {};
	self_info = WorldActivityRankMap::get_account_rank(account_uuid,cluster_coord,world_activity->m_available_since);
	if(self_info.rank == 0){
		return Response(Msg::ERR_NO_IN_WORLD_ACTIVITY_RANK);
	}
	if(self_info.rewarded){
		return Response(Msg::ERR_WORLD_ACTIVITY_RANK_HAVE_REWARDED);
	}
	world_activity->reward_rank(cluster_coord,account_uuid);
	return Response();
}

PLAYER_SERVLET(Msg::CS_WorldActivityDropData, account, session, req){
	try{
		const auto world_activity_data =  Data::WorldActivity::get(req.unique_id);
		if(!world_activity_data){
			LOG_EMPERY_CENTER_WARNING("no world activity data ",req.unique_id);
			return Response(Msg::ERR_NO_WORLD_ACTIVITY) << req.unique_id;
		}
		auto rewards = world_activity_data->rewards;
		Msg::SC_WorldActivityDropData msg;
		msg.unique_id = req.unique_id;
		for(auto it = rewards.begin(); it != rewards.end(); ++it){
			auto &drop = *msg.drop.emplace(msg.drop.end());
			drop.collection_name = it->first;
			drop.count           = it->second;
		}
		LOG_EMPERY_CENTER_FATAL(msg);
		session->send(msg);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}
	return Response();
}

PLAYER_SERVLET(Msg::CS_ActivityRankAwardData, account, session, req){
	try{
		std::vector<boost::shared_ptr<const Data::ActivityAward>> ret;
		Data::ActivityAward::get_activity_award_all(req.unique_id,ret);
		if(ret.empty()){
			LOG_EMPERY_CENTER_WARNING("activity award empty???,activity_id = ",req.unique_id);
		}
		Msg::SC_ActivityRankAwardData msg;
		for(auto it = ret.begin(); it != ret.end(); ++it){
			auto &rank_award_data = *it;
			auto &rank_list = *msg.rank_list.emplace(msg.rank_list.end());
			rank_list.begin = rank_award_data->rank_begin;
			rank_list.end   = rank_award_data->rank_end;
			for(auto itr = rank_award_data->rewards.begin(); itr != rank_award_data->rewards.end(); ++itr){
				auto &rank_reward = *rank_list.reward.emplace(rank_list.reward.end());
				rank_reward.item_id = (*itr).first;
				rank_reward.count = (*itr).second;
			}
		}
		LOG_EMPERY_CENTER_FATAL(msg);
		session->send(msg);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}
	return Response();
}


}
