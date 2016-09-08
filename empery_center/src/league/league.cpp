#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/ls_league.hpp"
#include "../msg/sl_league.hpp"
#include "../msg/sc_league.hpp"
#include "../msg/err_legion.hpp"
#include "../singletons/player_session_map.hpp"
#include "../player_session.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../legion.hpp"
#include "../singletons/legion_map.hpp"
#include "../legion_attribute_ids.hpp"
#include "../singletons/legion_member_map.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../data/item.hpp"
#include "../item_ids.hpp"
#include "../data/legion_corps_level.hpp"
#include "../legion_attribute_ids.hpp"
#include "../legion_member.hpp"
#include <poseidon/async_job.hpp>


namespace EmperyCenter {

namespace {
	std::pair<long, std::string> LeagueRegisterServerServlet(const boost::shared_ptr<LeagueClient> &server, Poseidon::StreamBuffer payload){
		PROFILE_ME;
		Msg::LS_LeagueRegisterServer req(payload);
		LOG_EMPERY_CENTER_TRACE("Received request from ", server->get_remote_info(), ": ", req);
// ============================================================================
{
	(void)req;

//	DungeonMap::add_server(server);

	return Response();
}
// ============================================================================
	}
	MODULE_RAII(handles){
		handles.push(LeagueClient::create_servlet(Msg::LS_LeagueRegisterServer::ID, &LeagueRegisterServerServlet));
	}
}


LEAGUE_SERVLET(Msg::LS_LeagueInfo, server, req){

	LOG_EMPERY_CENTER_DEBUG("LS_LeagueInfo=================== ", req.league_name);

	const auto account_uuid = AccountUuid(req.account_uuid);

	const auto account = AccountMap::get(account_uuid);
	if(account)
	{
		const auto target_session = PlayerSessionMap::get(account_uuid);
		if(target_session)
		{

			Msg::SC_LeagueInfo msg;
			msg.league_uuid = req.league_uuid;
			msg.league_name = req.league_name;
			msg.league_icon = req.league_icon;
			msg.league_notice = req.league_notice;
			msg.league_level = req.league_level;
			msg.leader_uuid = req.leader_legion_uuid;

			const auto leader_account = AccountMap::get(AccountUuid(req.create_account_uuid));
			if(leader_account)
				msg.leader_name = leader_account->get_nick();

			msg.league_titleid = req.legion_titleid;

			msg.members.reserve(req.members.size());
			for(auto it = req.members.begin(); it != req.members.end(); ++it )
			{
				auto &elem = *msg.members.emplace(msg.members.end());
				auto info = *it;

				elem.legion_uuid = info.legion_uuid;

				const auto legion = LegionMap::get(LegionUuid(info.legion_uuid));
				if(legion)
				{
					elem.legion_name = legion->get_nick();
					elem.legion_icon = legion->get_attribute(LegionAttributeIds::ID_ICON);
				}

				elem.speakflag = info.speakflag;
				elem.titleid = info.titleid;
			}

			target_session->send(msg);
		}
	}

	return Response();
}

LEAGUE_SERVLET(Msg::LS_Leagues, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_Leagues=================== ", req.account_uuid);

	const auto account_uuid = AccountUuid(req.account_uuid);

	const auto account = AccountMap::get(account_uuid);
	if(account)
	{
		const auto target_session = PlayerSessionMap::get(account_uuid);
		if(target_session)
		{
			Msg::SC_Leagues msg;

			msg.leagues.reserve(req.leagues.size());
			for(auto it = req.leagues.begin(); it != req.leagues.end(); ++it )
			{
				auto &elem = *msg.leagues.emplace(msg.leagues.end());
				auto info = *it;

				elem.league_uuid = info.league_uuid;
				elem.league_name = info.league_name;
				elem.league_icon = info.league_icon;
				elem.league_notice = info.league_notice;

				const auto leader_account = AccountMap::get(AccountUuid(info.league_leader_uuid));
				if(leader_account)
					elem.league_leader_name = leader_account->get_nick();

				elem.autojoin = info.autojoin;
				elem.league_create_time = info.league_create_time;
				elem.isapplyjoin = info.isapplyjoin;

				elem.legion_count = info.legions.size();

				for(auto lit = info.legions.begin(); lit != info.legions.end();++lit)
				{
					elem.totalmembercount += LegionMemberMap::get_legion_member_count(LegionUuid((*lit).legion_uuid));
				}
			}

			target_session->send(msg);
		}
	}

	return Response();
}

LEAGUE_SERVLET(Msg::LS_CreateLeagueRes, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_CreateLeagueRes=================== ", req.account_uuid);

	const auto account_uuid = AccountUuid(req.account_uuid);

	const auto account = AccountMap::get(account_uuid);
	if(account)
	{
		// 判断创建条件是否满足
		/*
		const auto item_box = ItemBoxMap::require(account_uuid);
		std::vector<ItemTransactionElement> transaction;
		const auto levelinfo = Data::LegionCorpsLevel::get(LegionCorpsLevelId(1));
		if(levelinfo)
		{
			for(auto it = levelinfo->level_up_cost_map.begin(); it != levelinfo->level_up_cost_map.end();++it)
			{
				if(it->first == boost::lexical_cast<std::string>(ItemIds::ID_DIAMONDS))
				{
					transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemIds::ID_DIAMONDS, it->second,
						ReasonIds::ID_CREATE_LEAGUE_FAIL, 0, 0, it->second);
				}
			}
		}

		// 恢复资源
		const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
			[&]{
				LOG_EMPERY_CENTER_ERROR("LS_CreateLeagueRes=================== ", req.res);
			});

			if(insuff_item_id)
			{
				LOG_EMPERY_CENTER_ERROR("LS_CreateLeagueRes=================== ", req.res);
			}


		const auto target_session = PlayerSessionMap::get(account_uuid);
		if(target_session)
		{
			Msg::SC_CreateLeagueRes msg;
			msg.res = req.res;
			target_session->send(msg);
		}

		*/
	}

	return Response();
}

LEAGUE_SERVLET(Msg::LS_ApplyJoinList, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_ApplyJoinList=================== ", req.account_uuid);

	const auto account_uuid = AccountUuid(req.account_uuid);

	const auto account = AccountMap::get(account_uuid);
	if(account)
	{
		const auto target_session = PlayerSessionMap::get(account_uuid);
		if(target_session)
		{

			Msg::SC_ApplyJoinList msg;
			unsigned size = 0;
			for(auto it = req.applies.begin(); it != req.applies.end(); ++it )
			{
				auto info = *it;

				const auto& legion = LegionMap::get(LegionUuid(info.legion_uuid));
				if(legion)
				{
					auto &elem = *msg.applies.emplace(msg.applies.end());

					elem.legion_uuid = legion->get_legion_uuid().str();
					elem.legion_name = legion->get_nick();
					elem.icon = legion->get_attribute(LegionAttributeIds::ID_ICON);
					elem.level = legion->get_attribute(LegionAttributeIds::ID_LEVEL);

					const auto leader_account = AccountMap::get(AccountUuid(legion->get_attribute(LegionAttributeIds::ID_LEADER)));
					if(leader_account)
						elem.leader_name = leader_account->get_nick();

					size += 1;
				}
			}

			msg.applies.reserve(size);

			target_session->send(msg);
		}
	}

	return Response();
}

LEAGUE_SERVLET(Msg::LS_InvieJoinList, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_InvieJoinList=================== ", req.account_uuid);

	Msg::SC_InvieJoinList msg;
	msg.invites.reserve(req.invites.size());
	for(auto it = req.invites.begin(); it != req.invites.end(); ++it )
	{
		auto &elem = *msg.invites.emplace(msg.invites.end());
		auto info = *it;

		elem.league_uuid = info.league_uuid;
		elem.league_name = info.league_name;
		elem.league_icon = info.league_icon;

		const auto leader_account = AccountMap::get(AccountUuid(info.league_leader_uuid));
		if(leader_account)
			elem.leader_name = leader_account->get_nick();
		else
			elem.leader_name = "";
	}

	const auto account_uuid = AccountUuid(req.account_uuid);

	const auto account = AccountMap::get(account_uuid);
	if(account)
	{
		const auto target_session = PlayerSessionMap::get(account_uuid);
		if(target_session)
		{
			target_session->send(msg);
		}
	}
	else
	{
		// 发送给军团邀请加入联盟的

	}

	return Response();
}

LEAGUE_SERVLET(Msg::LS_ExpandLeagueReq, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_ExpandLeagueReq=================== ", req.account_uuid);

	const auto account_uuid = AccountUuid(req.account_uuid);

	const auto account = AccountMap::get(account_uuid);
	if(account)
	{
		// 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEGION_NOT_JOIN);
        }

		const auto item_box = ItemBoxMap::require(account_uuid);
		std::vector<ItemTransactionElement> transaction;
		for(auto it = req.consumes.begin(); it != req.consumes.end(); ++it )
		{
			auto &info = *it; 

			if(info.consue_type == boost::lexical_cast<std::string>(ItemIds::ID_DIAMONDS))
			{
				// 玩家当前拥有的钻石 (这个数值比前端现实的大100倍) / 100
				const auto  curDiamonds = item_box->get(ItemIds::ID_DIAMONDS).count;
				LOG_EMPERY_CENTER_DEBUG("CS_LegionCreateReqMessage account_uuid:", account_uuid,"  curDiamonds:",curDiamonds);
				if(curDiamonds < info.num)
				{
					return Response(Msg::ERR_LEGION_CREATE_NOTENOUGH_MONEY);
				}
				else
				{
					transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ItemIds::ID_DIAMONDS, info.num,
						ReasonIds::ID_EXPAND_LEAGUE, 0, 0, info.num);
				}
			}
			else
			{
				return Response(Msg::ERR_LEGION_CREATE_NOTENOUGH_MONEY);
			}

		}

		// 扣除费用，并发消息去联盟服务器
		const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{
			Msg::SL_ExpandLeagueRes msg;
			msg.account_uuid = req.account_uuid;
			msg.legion_uuid = member->get_legion_uuid().str();

			const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_LeagueCreateReqMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);
		});

		if(insuff_item_id)
		{
			return Response(Msg::ERR_LEGION_CREATE_NOTENOUGH_MONEY);
		}
	}

	return Response();
}


}
