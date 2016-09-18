#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/ls_league.hpp"
#include "../msg/sl_league.hpp"
#include "../msg/sc_league.hpp"
#include "../msg/err_legion.hpp"
#include "../msg/err_chat.hpp"
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
#include "../account_attribute_ids.hpp"
#include "../chat_channel_ids.hpp"
#include "../chat_message_type_ids.hpp"
#include "../chat_message_slot_ids.hpp"
#include "../chat_box.hpp"
#include "../chat_message.hpp"
#include "../singletons/chat_box_map.hpp"
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
		if(req.rewrite == 1)
		{
			boost::container::flat_map<AccountAttributeId, std::string> Attributes;

			Attributes[AccountAttributeIds::ID_LEAGUE_UUID] = req.league_uuid;

			account->set_attributes(std::move(Attributes));
		}
		if(req.res == 0)
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
				msg.league_max_member = req.league_max_member;
				msg.leader_uuid = req.leader_legion_uuid;
				msg.leader_name = "";
				const auto& leader_legion = LegionMap::get(LegionUuid(req.leader_legion_uuid));
				if(leader_legion)
				{
					const auto& leader_account = AccountMap::get(AccountUuid(leader_legion->get_attribute(LegionAttributeIds::ID_LEADER)));
					if(leader_account)
						msg.leader_name = leader_account->get_nick();
				}

				/*
				const auto leader_account = AccountMap::get(AccountUuid(req.create_account_uuid));
				if(leader_account)
					msg.leader_name = leader_account->get_nick();
				*/

				msg.members.reserve(req.members.size());
				for(auto it = req.members.begin(); it != req.members.end(); ++it )
				{
					auto &elem = *msg.members.emplace(msg.members.end());
					auto info = *it;

					elem.legion_uuid = info.legion_uuid;

					const auto& legion = LegionMap::get(LegionUuid(info.legion_uuid));
					if(legion)
					{
						elem.legion_name = legion->get_nick();
						elem.legion_icon = legion->get_attribute(LegionAttributeIds::ID_ICON);

						const auto& leader_account = AccountMap::get(AccountUuid(legion->get_attribute(LegionAttributeIds::ID_LEADER)));
						if(leader_account)
							elem.legion_leader_name = leader_account->get_nick();

					}

					elem.titleid = info.titleid;
					elem.quit_time = info.quit_time;
					elem.kick_time = info.kick_time;
					elem.attorn_time = info.attorn_time;
				}

				target_session->send(msg);
			}
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

				/*
				const auto leader_account = AccountMap::get(AccountUuid(info.league_leader_uuid));
				if(leader_account)
					elem.league_leader_name = leader_account->get_nick();
				*/
				elem.league_leader_name = "";
				const auto& leader_legion = LegionMap::get(LegionUuid(info.league_leader_uuid));
				if(leader_legion)
				{
					const auto& leader_account = AccountMap::get(AccountUuid(leader_legion->get_attribute(LegionAttributeIds::ID_LEADER)));
					if(leader_account)
						elem.league_leader_name = leader_account->get_nick();
				}

				elem.autojoin = info.autojoin;
				elem.league_create_time = info.league_create_time;
				elem.isapplyjoin = info.isapplyjoin;
				elem.legion_max_count =info.legion_max_count;
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

		const auto& leader_legion = LegionMap::get(LegionUuid(info.league_leader_uuid));
		if(leader_legion)
		{
			const auto& leader_account = AccountMap::get(AccountUuid(leader_legion->get_attribute(LegionAttributeIds::ID_LEADER)));
			if(leader_account)
				elem.leader_name = leader_account->get_nick();
		}
		else
		{
			elem.leader_name = "";
		}
		/*
		const auto leader_account = AccountMap::get(AccountUuid(info.league_leader_uuid));
		if(leader_account)
			elem.leader_name = leader_account->get_nick();
		else
			elem.leader_name = "";

			*/
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
		const auto& legion = LegionMap::get(LegionUuid(req.legion_uuid));
		if(legion)
		{
			legion->send_msg_by_power(msg,Legion::LEGION_POWER::LEGION_POWER_LEAGUE_INVITE);
		}
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


LEAGUE_SERVLET(Msg::LS_AttornLeagueRes, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_AttornLeagueRes=================== ", req.legion_uuid);

	const auto& legion = LegionMap::get(LegionUuid(req.legion_uuid));
	if(legion)
	{
		//
	}
	else
	{
		return Response(Msg::ERR_LEGION_CANNOT_FIND);
	}

	return Response();
}

LEAGUE_SERVLET(Msg::LS_banChatLeagueReq, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_banChatLeagueReq=================== ", req.account_uuid);

	const auto account_uuid = AccountUuid(req.account_uuid);

	const auto account = AccountMap::get(account_uuid);
	if(account)
	{
		// 设置account的联盟聊天禁言标识

		boost::container::flat_map<AccountAttributeId, std::string> Attributes;

		Attributes[AccountAttributeIds::ID_LEAGUE_CAHT_FALG] = boost::lexical_cast<std::string>(req.bban);

		account->set_attributes(std::move(Attributes));
	}
	else
	{
		return Response(Msg::ERR_LEGION_CANNOT_FIND);
	}

	return Response();
}

LEAGUE_SERVLET(Msg::LS_LeagueChat, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_LeagueChat=================== ", req.account_uuid);

	const auto account_uuid = AccountUuid(req.account_uuid);

	const auto account = AccountMap::get(account_uuid);
	if(account)
	{
		// 

		const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(member)
        {
			std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
			segments.reserve(req.segments.size());
			for(auto it = req.segments.begin(); it != req.segments.end(); ++it){
				const auto slot = ChatMessageSlotId(it->slot);
				if((slot != ChatMessageSlotIds::ID_TEXT) && (slot != ChatMessageSlotIds::ID_SMILEY) && (slot != ChatMessageSlotIds::ID_VOICE)){
					return Response(Msg::ERR_INVALID_CHAT_MESSAGE_SLOT) <<slot;
				}
				segments.emplace_back(slot, std::move(it->value));
			}

			const auto message = boost::make_shared<ChatMessage>(
				ChatMessageUuid(req.chat_message_uuid), ChatChannelId(req.channel), ChatMessageTypeId(req.type), LanguageId(req.language_id), req.created_time, AccountUuid(req.account_uuid), std::move(segments));

           for(auto it = req.legions.begin(); it != req.legions.end(); ++it )
			{
				auto info = *it;

				const auto& legion = LegionMap::get(LegionUuid(info.legion_uuid));
				if(legion)
				{
					// 根据account_uuid查找是否有军团
					std::vector<boost::shared_ptr<LegionMember>> members;
					LegionMemberMap::get_by_legion_uuid(members, legion->get_legion_uuid());

					LOG_EMPERY_CENTER_INFO("get_by_legion_uuid legion members size*******************",members.size());

					if(!members.empty())
					{

						for(auto it = members.begin(); it != members.end(); ++it )
						{
							auto info = *it;

							// 判断是否在线
							const auto target_session = PlayerSessionMap::get(AccountUuid(info->get_account_uuid()));
							if(target_session)
							{
								try {
								Poseidon::enqueue_async_job(
									[=]() mutable {
										PROFILE_ME;
										const auto other_chat_box = ChatBoxMap::require(info->get_account_uuid());
										other_chat_box->insert(message);

										LOG_EMPERY_CENTER_INFO("LegionMemberMap::chat 有人说话  内容已转发============");
									});
								} catch(std::exception &e){
									LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
									target_session->shutdown(e.what());
								}
							}
						}
					}
				}
			}
        }
	}
	else
	{
		return Response(Msg::ERR_LEGION_CANNOT_FIND);
	}

	return Response();
}

LEAGUE_SERVLET(Msg::LS_LeagueNoticeMsg, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_LeagueNoticeMsg=================== ", req.msgtype);

	Msg::SC_LeagueNoticeMsg msg;
	msg.msgtype = req.msgtype;
	msg.nick = req.nick;
	msg.ext1 = req.ext1;

	if(msg.msgtype == 6 || msg.msgtype == 9)
	{
		const auto& legion = LegionMap::get(LegionUuid(msg.nick));
		if(legion)
		{
			msg.nick = legion->get_nick();

			if(msg.msgtype == 9)
			{
				legion->set_member_league_uuid("");
			}
		}
	}
	if(msg.msgtype == 1 || msg.msgtype == 2 || msg.msgtype == 3 || msg.msgtype == 6)
	{
		const auto& legion = LegionMap::get(LegionUuid(req.ext1));
		if(legion)
		{
			msg.ext1 = legion->get_nick();

			if(msg.msgtype == 1)
			{
				legion->set_member_league_uuid(req.league_uuid);
			}
			else if( msg.msgtype == 2 || msg.msgtype == 3 )
			{
				legion->set_member_league_uuid("");
			}
		}
	}
	else if(msg.msgtype == 3 || msg.msgtype == 4)
	{
		const auto& legion = LegionMap::get(LegionUuid(req.ext1));
		if(legion)
		{
			msg.ext1 = legion->get_nick();

			if(msg.msgtype == 3)
			{
				// 军团别踢出联盟 广播给军团其他成员
				Msg::SC_LegionNoticeMsg msg;
				msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_LEAGUE_KICK;
				msg.nick = msg.nick;
				msg.ext1 = "";
				legion->sendNoticeMsg(msg);
			}

			const auto& leader_account = AccountMap::get(AccountUuid(legion->get_attribute(LegionAttributeIds::ID_LEADER)));
			if(leader_account)
				msg.nick = leader_account->get_nick();
		}
	}

	for(auto it = req.legions.begin(); it != req.legions.end(); ++it)
	{
		auto info = *it;

		const auto& legion = LegionMap::get(LegionUuid(info.legion_uuid));
		if(legion)
		{
			legion->broadcast_to_members(msg);
		}
	}

	return Response();
}

LEAGUE_SERVLET(Msg::LS_LeagueEamilMsg, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_LeagueEamilMsg=================== ");

	std::string strcontent = req.ext1;
	const auto typeID = ChatMessageTypeId(boost::lexical_cast<unsigned int>(req.ntype));
	if(typeID == EmperyCenter::ChatMessageTypeIds::ID_LEVEL_LEAGUE_REFUSE_APPLY)
	{
		// 拒绝申请加入
		const auto& account = AccountMap::get(AccountUuid(req.account_uuid));
		if(account)
		{
			const auto& mandator_account = AccountMap::get(AccountUuid(req.mandator));
			if(mandator_account)
				strcontent += "," + mandator_account->get_nick();

			const auto& member = LegionMemberMap::get_by_account_uuid(account->get_account_uuid());
			if(member)
			{
				const auto& legion = LegionMap::get(member->get_legion_uuid());
				if(legion)
				{
					legion->sendmail(account, typeID, strcontent);
				}

				return Response();
			}
		}
	}
	else if(typeID == EmperyCenter::ChatMessageTypeIds::ID_LEVEL_LEAGUE_REFUSE_INVITE)
	{
		// 拒绝邀请加入
		const auto& account = AccountMap::get(AccountUuid(req.account_uuid));
		if(account)
		{
			const auto& legion = LegionMap::get(LegionUuid(req.legion_uuid));
			if(legion)
			{
				strcontent = legion->get_nick();
			}
			const auto& mandator_account = AccountMap::get(AccountUuid(req.mandator));
			if(mandator_account)
				strcontent += "," + mandator_account->get_nick();

			legion->sendmail(account, typeID, strcontent);

			return Response();
		}
	}
	else if(typeID == EmperyCenter::ChatMessageTypeIds::ID_LEVEL_LEAGUE_KICK)
	{
		const auto& leader_account = AccountMap::get(AccountUuid(req.mandator));
		if(leader_account)
			strcontent += "," + leader_account->get_nick();
	}
	const auto& legion = LegionMap::get(LegionUuid(req.legion_uuid));
	if(legion)
	{
		std::vector<boost::shared_ptr<LegionMember>> members;
		LegionMemberMap::get_by_legion_uuid(members, legion->get_legion_uuid());
		if(!members.empty())
		{
			for(auto it = members.begin(); it != members.end(); ++it)
			{
				auto& member = *it;
				const auto& account = AccountMap::get(member->get_account_uuid());
				if(account)
				{
					legion->sendmail(account, typeID, strcontent);
				}
			}
		}
	}

	return Response();
}

LEAGUE_SERVLET(Msg::LS_LookLeagueLegionsReq, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_LookLeagueLegionsReq=================== ", req.account_uuid);

	const auto account_uuid = AccountUuid(req.account_uuid);

	const auto account = AccountMap::get(account_uuid);
	if(account)
	{
		const auto target_session = PlayerSessionMap::get(account_uuid);
		if(target_session)
		{

			Msg::SC_LookLeagueLegions msg;
			unsigned int ncount = 0;
			for(auto it = req.legions.begin(); it != req.legions.end(); ++it )
			{
				auto &elem = *msg.legions.emplace(msg.legions.end());
				auto info = *it;

				const auto& legion = LegionMap::get(LegionUuid(info.legion_uuid));
				if(legion)
				{
					elem.legion_name = legion->get_nick();
					elem.legion_icon = legion->get_attribute(LegionAttributeIds::ID_ICON);
					const auto& leader_account = AccountMap::get(AccountUuid(legion->get_attribute(LegionAttributeIds::ID_LEADER)));
					if(leader_account)
						elem.legion_leader_name = leader_account->get_nick();
					else
						elem.legion_leader_name = "";

					ncount += 1;
				}

			}

			msg.legions.reserve(ncount);

			target_session->send(msg);
		}
	}

	return Response();
}

LEAGUE_SERVLET(Msg::LS_disbandLegionRes, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_disbandLegionRes=================== ", req.account_uuid);



	const auto& legion = LegionMap::get(LegionUuid(req.legion_uuid));
	if(legion)
	{
		const auto account_uuid = AccountUuid(req.account_uuid);

		const auto account = AccountMap::get(account_uuid);
		if(account)
		{
			// 发邮件告诉结果
			legion->sendmail(account,ChatMessageTypeIds::ID_LEVEL_LEGION_DISBAND,legion->get_nick());
		}

		// 解散军团
		LegionMap::deletelegion(legion->get_legion_uuid());

		return Response(Msg::ST_OK);
	}

	return Response();
}


LEAGUE_SERVLET(Msg::LS_disbandLeagueRes, server, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("LS_disbandLeagueRes=================== ", req.account_uuid);

	const auto& legion = LegionMap::get(LegionUuid(req.legion_uuid));
	if(legion)
	{
		legion->set_member_league_uuid("");

		return Response(Msg::ST_OK);
	}

	return Response();
}




}
