#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_legion.hpp"
#include "../mysql/legion.hpp"
#include "../msg/err_legion.hpp"
#include "../legion.hpp"
#include "../singletons/legion_map.hpp"
#include <poseidon/json.hpp>
#include <poseidon/async_job.hpp>
#include <string>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../singletons/chat_box_map.hpp"
#include "../account.hpp"
#include "../account_attribute_ids.hpp"
#include "../data/global.hpp"
#include "../singletons/world_map.hpp"
#include "../horn_message.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../data/item.hpp"
#include "../item_ids.hpp"
#include "../singletons/player_session_map.hpp"
#include "../singletons/legion_member_map.hpp"
#include "../id_types.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../legion_member.hpp"
#include "../legion_member_attribute_ids.hpp"
#include "../msg/sc_legion.hpp"
#include "../legion_attribute_ids.hpp"
#include "../singletons/legion_applyjoin_map.hpp"
#include "../singletons/legion_invitejoin_map.hpp"
#include "../data/legion_corps_power.hpp"
#include "../data/legion_corps_level.hpp"
#include "../data/legion_store_config.hpp"
#include "../data/global.hpp"
#include "../castle.hpp"
#include "../chat_message_type_ids.hpp"
#include "../attribute_ids.hpp"
#include "../task_box.hpp"
#include "../task_type_ids.hpp"
#include "../singletons/task_box_map.hpp"
#include "../legion_task_box.hpp"
#include "../singletons/legion_task_box_map.hpp"
#include "../legion_task_contribution_box.hpp"
#include "../singletons/legion_task_contribution_box_map.hpp"
#include "../msg/sl_league.hpp"
#include "../singletons/league_client.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_LegionCreateReqMessage, account, session, req){
	PROFILE_ME;

	const auto name     	= 	req.name;
	const auto icon        	= 	req.icon;
	const auto content       = 	req.content;
	const auto language 	= 	req.language;
	const auto bshenhe 		= 	req.bshenhe;

	/*
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < account->get_quieted_until()){
		return Response(Msg::ERR_ACCOUNT_QUIETED);
	}
	*/

	const auto account_uuid = account->get_account_uuid();

	LOG_EMPERY_CENTER_DEBUG("CS_LegionCreateReqMessage name:", name," icon:",icon, " content:",content,"language:",language," bshenhe:",bshenhe);
	// 判断玩家是否已经有军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		return Response(Msg::ERR_ACCOUNT_HAVE_LEGION);
	}
	auto count = LegionMap::get_count();
	LOG_EMPERY_CENTER_INFO("CS_LegionCreateReqMessage count:", count);

	// 判断是否已经存在同名的军团
	std::vector<boost::shared_ptr<Legion>> other_legiones;
	LegionMap::get_by_nick(other_legiones,std::move(name));
	if(!other_legiones.empty())
	{
		LOG_EMPERY_CENTER_DEBUG("CS_LegionCreateReqMessage name = ",name);
		return Response(Msg::ERR_LEGION_CREATE_HOMONYM);
	}


	// 判断创建条件是否满足
	const auto primary_castle =  WorldMap::get_primary_castle(account_uuid);
	if(!primary_castle || primary_castle->get_level() < Data::Global::as_unsigned(Data::Global::SLOT_LEGION_MAINCITY_LOARD_LEVEL_LIMIT))
	{
		return Response(Msg::ERR_LEGION_MAINCITY_LOARD_LEVEL_LIMIT);
	}

	const auto item_box = ItemBoxMap::require(account_uuid);
	std::vector<ItemTransactionElement> transaction;
	const auto levelinfo = Data::LegionCorpsLevel::get(LegionCorpsLevelId(1));
	if(levelinfo)
	{
		for(auto it = levelinfo->level_up_cost_map.begin(); it != levelinfo->level_up_cost_map.end();++it)
		{
			if(it->first == boost::lexical_cast<std::string>(ItemIds::ID_DIAMONDS))
			{
				const auto  curDiamonds = item_box->get(ItemIds::ID_DIAMONDS).count;
				LOG_EMPERY_CENTER_DEBUG("CS_LegionCreateReqMessage account_uuid:", account_uuid,"  curDiamonds:",curDiamonds);
				if(curDiamonds < it->second)
				{
					return Response(Msg::ERR_LEGION_CREATE_NOTENOUGH_MONEY);
				}
				else
				{
					transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ItemIds::ID_DIAMONDS, it->second,
						ReasonIds::ID_CREATE_LEGION, 0, 0, it->second);
				}
			}
			else
			{
				// 玩家当前拥有的金币 (这个数值比前端现实的大100倍) / 100
				//	const auto  curmoney = item_box->get(ItemIds::ID_GOLD_COINS).count;
				LOG_EMPERY_CENTER_ERROR("CS_LegionCreateReqMessage: type unsupport  =================================================== ", it->first);
				return Response(Msg::ERR_LEGION_CREATE_NOTENOUGH_MONEY);
			}
		}
	}

	const auto utc_now = Poseidon::get_utc_time();

	LOG_EMPERY_CENTER_DEBUG("CS_LegionCreateReqMessage account_uuid:", account_uuid,"  utc_now:",utc_now);

	// 创建军团并插入到军团列表内存中
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{
			const auto legion_uuid = LegionUuid(Poseidon::Uuid::random());
			auto pair = Legion::async_create(legion_uuid,std::move(name),account_uuid, utc_now);
			Poseidon::JobDispatcher::yield(pair.first, true);

			auto legion = std::move(pair.second);
			// 初始化军团的一些属性
			legion->InitAttributes(account_uuid,std::move(content), std::move(language), std::move(icon),bshenhe);

			// 根据配置文件取得职位id

			// 增加军团成员
		//	legion->AddMember(account_uuid,1,utc_now,account->get_attribute(AccountAttributeIds::ID_DONATE),account->get_attribute(AccountAttributeIds::ID_WEEKDONATE),account->get_nick());

			Msg::SC_LegionNoticeMsg msg;
			msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_CREATE_SUCCESS;
			msg.nick = "";
			msg.ext1 = "";
			session->send(msg);

			legion->AddMember(account,1,utc_now);

			LegionMap::insert(legion, std::string());

			count = LegionMap::get_count();
			LOG_EMPERY_CENTER_INFO("CS_LegionCreateReqMessage count:", count);

			legion->synchronize_with_player(account_uuid,session);
		});

		if(insuff_item_id)
		{
			return Response(Msg::ERR_LEGION_CREATE_NOTENOUGH_MONEY);
		}


	return Response(Msg::ST_OK);
}

PLAYER_SERVLET(Msg::CS_GetLegionBaseInfoMessage, account, session, req){
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	LOG_EMPERY_CENTER_DEBUG("CS_GetLegionBaseInfoMessage account_uuid ================================ ",account_uuid);

	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		LOG_EMPERY_CENTER_DEBUG("CS_GetLegionBaseInfoMessage member.legion_uuid = ",member->get_legion_uuid());
		// 根据account_uuid查找是否有军团
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			legion->synchronize_with_player(account_uuid,session);

			return Response(Msg::ST_OK);
		}
		else
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
	}
	else
	{
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}


PLAYER_SERVLET(Msg::CS_GetLegionMemberInfoMessage, account, session, req){
	PROFILE_ME;

	LOG_EMPERY_CENTER_INFO("CS_GetLegionMemberInfoMessage");

	const auto account_uuid = account->get_account_uuid();

	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 根据account_uuid查找是否有军团
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			// 根据account_uuid查找是否有军团
			std::vector<boost::shared_ptr<LegionMember>> members;
			LegionMemberMap::get_by_legion_uuid(members, member->get_legion_uuid());

			LOG_EMPERY_CENTER_INFO("get_by_legion_uuid legion members size*******************",members.size());

			if(!members.empty())
			{
				Msg::SC_LegionMembers msg;
				msg.members.reserve(members.size());
				for(auto it = members.begin(); it != members.end(); ++it )
				{
					auto &elem = *msg.members.emplace(msg.members.end());
					auto info = *it;

					elem.account_uuid = info->get_account_uuid().str();
					elem.speakflag = info->get_attribute(LegionMemberAttributeIds::ID_SPEAKFLAG);
					elem.titleid = info->get_attribute(LegionMemberAttributeIds::ID_TITLEID);

					const auto legionmember = AccountMap::require(AccountUuid(elem.account_uuid));

					elem.nick = legionmember->get_nick();
					elem.icon = legionmember->get_attribute(AccountAttributeIds::ID_AVATAR);

					const auto primary_castle =  WorldMap::get_primary_castle(info->get_account_uuid());
					if(primary_castle)
					{
						elem.prosperity = boost::lexical_cast<std::uint64_t>(primary_castle->get_attribute(AttributeIds::ID_PROSPERITY_POINTS));

						LOG_EMPERY_CENTER_INFO("城堡繁荣度==============================================",elem.prosperity, " 用户：",elem.account_uuid);
					}
					else
					{
						LOG_EMPERY_CENTER_INFO("城堡繁荣度 没找到==============================================");
						elem.prosperity = 0;   //ID_PROSPERITY_POINTS
					}

					elem.fighting = "0";

					elem.quit_time = info->get_attribute(LegionMemberAttributeIds::ID_QUITWAITTIME);
					elem.kick_time = info->get_attribute(LegionMemberAttributeIds::ID_KICKWAITTIME);
					if(info->get_account_uuid().str() == legion->get_attribute(LegionAttributeIds::ID_ATTORNLEADER))
						elem.attorn_time = legion->get_attribute(LegionAttributeIds::ID_ATTORNTIME);
					else
						elem.attorn_time = "";
				}

				LOG_EMPERY_CENTER_INFO("legion members size==============================================",msg.members.size());
				session->send(msg);

				return Response(Msg::ST_OK);
			}
			else
			{
				return Response(Msg::ERR_LEGION_CANNOT_FIND);
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}
	else
	{
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_GetAllLegionMessage, account, session, req){
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();
	std::vector<boost::shared_ptr<Legion>> legions;
	LegionMap::get_all(legions, 0, 1000);

	Msg::SC_Legions msg;
	msg.legions.reserve(legions.size());

	for(auto it = legions.begin(); it != legions.end(); ++it )
	{
		auto &elem = *msg.legions.emplace(msg.legions.end());
		auto legion = *it;

		elem.legion_uuid = legion->get_legion_uuid().str();
		elem.legion_name = legion->get_nick();
		elem.legion_icon = legion->get_attribute(LegionAttributeIds::ID_ICON);

		LOG_EMPERY_CENTER_INFO("CS_GetAllLegionMessage elem.legion_uuid:", elem.legion_uuid);
		const auto leader_account = AccountMap::require(AccountUuid(legion->get_attribute(LegionAttributeIds::ID_LEADER)));
		elem.legion_leadername = leader_account->get_nick();
		elem.level = legion->get_attribute(LegionAttributeIds::ID_LEVEL);

		elem.membercount = boost::lexical_cast<std::string>(LegionMemberMap::get_legion_member_count(legion->get_legion_uuid()));
		elem.autojoin = legion->get_attribute(LegionAttributeIds::ID_AUTOJOIN);
		elem.rank = legion->get_attribute(LegionAttributeIds::ID_RANK);
		elem.notice = legion->get_attribute(LegionAttributeIds::ID_CONTENT);

		const auto primary_castle =  WorldMap::get_primary_castle(leader_account->get_account_uuid());
		if(primary_castle)
		{
			elem.legion_Leader_maincity_loard_level =  boost::lexical_cast<std::string>(primary_castle->get_level());
		}
		else
		{
			elem.legion_Leader_maincity_loard_level =  "0";
		}
		elem.legion_create_time =  boost::lexical_cast<std::string>(legion->get_created_time());

		// 是否请求加入过
		const auto apply = LegionApplyJoinMap::find(account_uuid,legion->get_legion_uuid());
		if(apply)
		{
			elem.isapplyjoin = "1";
		}
		else
		{
			elem.isapplyjoin = "0";
		}
	}

	LOG_EMPERY_CENTER_INFO("CS_GetAllLegionMessage size==============================================",msg.legions.size());

	session->send(msg);

	return Response(Msg::ST_OK);
}

PLAYER_SERVLET(Msg::CS_ApplyJoinLegionMessage, account, session, req){
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);

	const auto primary_castle =  WorldMap::get_primary_castle(account_uuid);
	if(!primary_castle || primary_castle->get_level() < Data::Global::as_unsigned(Data::Global::SLOT_LEGION_MAINCITY_LOARD_LEVEL_LIMIT))
	{
		return Response(Msg::ERR_LEGION_MAINCITY_LOARD_LEVEL_LIMIT);
	}

	if(member)
	{
		// 已加入军团
		return Response(Msg::ERR_ACCOUNT_HAVE_LEGION);
	}
	else
	{
		const auto legion = LegionMap::get(LegionUuid(req.legion_uuid));
		if(legion)
		{
			// 是否已经申请过
			const auto apply = LegionApplyJoinMap::find(account_uuid,LegionUuid(req.legion_uuid));
			if(apply)
			{
				if(req.bCancle)
				{
					// 取消申请
					LegionApplyJoinMap::deleteInfo(LegionUuid(req.legion_uuid),account_uuid,false);

					return Response(Msg::ST_OK);
				}
				else
				{
					return Response(Msg::ERR_LEGION_ALREADY_APPLY);
				}
			}
			else
			{
				if(req.bCancle)
				{
					return Response(Msg::ERR_LEGION_CANNOT_FIND_DATA);
				}
				else
				{
					// 检查下最大允许申请的数量
					if(LegionApplyJoinMap::get_apply_count(account_uuid) >= Data::Global::as_unsigned(Data::Global::SLOT_LEGION_ENABLE_APPLY_NUMBER))
					{
						return Response(Msg::ERR_LEGION_APPLY_COUNT_LIMIT);
					}
				}
			}

			// 成员数
			const auto levelinfo = Data::LegionCorpsLevel::get(LegionCorpsLevelId(boost::lexical_cast<uint32_t>(legion->get_attribute(LegionAttributeIds::ID_LEVEL))));
			std::uint64_t limit = 0;
			if(levelinfo)
			{
				limit = levelinfo->legion_member_max;
			}
			else
			{
				return Response(Msg::ERR_LEGION_CONFIG_CANNOT_FIND);
			}
			const auto count = LegionMemberMap::get_legion_member_count(legion->get_legion_uuid());
			if(count < limit)
			{
				const auto utc_now = Poseidon::get_utc_time();
				const auto autojoin = legion->get_attribute(LegionAttributeIds::ID_AUTOJOIN);
				if(autojoin == "0")
				{
					// 不用审核，直接加入

				//	legion->AddMember(account_uuid,Data::Global::as_unsigned(Data::Global::SLOT_LEGION_MEMBER_DEFAULT_POWERID),utc_now,account->get_attribute(AccountAttributeIds::ID_DONATE),account->get_attribute(AccountAttributeIds::ID_WEEKDONATE),account->get_nick());
					legion->AddMember(account,Data::Global::as_unsigned(Data::Global::SLOT_LEGION_MEMBER_DEFAULT_POWERID),utc_now);

					legion->synchronize_with_player(account_uuid,session);

					// 如果还请求加入过其他军团，也需要做善后操作，删除其他所有加入请求
					LegionApplyJoinMap::deleteInfo(LegionUuid(req.legion_uuid),account_uuid,true);

					// 别人邀请加入的也要清空
					LegionInviteJoinMap::deleteInfo_by_invited_uuid(account_uuid);

					// 发邮件告诉结果
					legion->sendmail(account,ChatMessageTypeIds::ID_LEVEL_LEGION_JOIN,legion->get_nick());
					/*
					Msg::SC_LegionEmailMessage msg;
					msg.legion_uuid = LegionUuid(req.legion_uuid).str();
					msg.legion_name = legion->get_nick();
					msg.ntype = Legion::LEGION_EMAIL_TYPE::LEGION_EMAIL_JOIN;
					session->send(msg);
					*/
					return Response(Msg::ST_OK);
				}
				else
				{
					// 添加到待审批的列表中
					auto obj = boost::make_shared<MySql::Center_LegionApplyJoin>( account_uuid.get(),LegionUuid(req.legion_uuid).get(),utc_now);
					obj->enable_auto_saving();
					auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);

					LegionApplyJoinMap::insert(obj);

					LOG_EMPERY_CENTER_DEBUG("CS_ApplyJoinLegionMessage apply_count = ",LegionApplyJoinMap::get_apply_count(account_uuid));


					return Response(Msg::ST_OK);
				}
			}
			else
			{
				return Response(Msg::ERR_LEGION_MEMBER_FULL);
			}

			   return Response(Msg::ST_OK);
			}
		else
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
	}

	return Response(Msg::ST_OK);
}

PLAYER_SERVLET(Msg::CS_GetApplyJoinLegionMessage, account, session, req)
{
	PROFILE_ME;
	const auto account_uuid = account->get_account_uuid();

	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 查看军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			// 查看member是否有权限
			const auto titileid = member->get_attribute(LegionMemberAttributeIds::ID_TITLEID);
			if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_APPROVE))
			{
				LegionApplyJoinMap::synchronize_with_player(member->get_legion_uuid(),session);
			}
			else
			{
				return Response(Msg::ERR_LEGION_NO_POWER);
			}


			return Response(Msg::ST_OK);
		}
		else
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_LegionAuditingResMessage, account, session, req)
{
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("CS_LegionAuditingResMessage req.agree ========================================= ",req.bagree);

	const auto account_uuid = account->get_account_uuid();
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 查看是否存在该申请
		const auto ApplyJoininfo = LegionApplyJoinMap::find(AccountUuid(req.account_uuid),LegionUuid(member->get_legion_uuid()));
		if(ApplyJoininfo)
		{
			// 查看军团是否存在
			const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
			if(legion)
			{
				// 查看对方是否已经加入军团
				const auto memberinfo = LegionMemberMap::get_by_account_uuid(AccountUuid(req.account_uuid));
				if(memberinfo)
				{
					return Response(Msg::ERR_ACCOUNT_HAVE_LEGION) << req.account_uuid;
				}

				// 查看member是否有权限
				const auto titileid = member->get_attribute(LegionMemberAttributeIds::ID_TITLEID);
				bool bdeleteAll = false;
				if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_APPROVE))
				{
					// 判断成员数
					const auto levelinfo = Data::LegionCorpsLevel::get(LegionCorpsLevelId(boost::lexical_cast<uint32_t>(legion->get_attribute(LegionAttributeIds::ID_LEVEL))));
					std::uint64_t limit = 0;
					if(levelinfo)
					{
						limit = levelinfo->legion_member_max;
					}
					else
					{
						return Response(Msg::ERR_LEGION_CONFIG_CANNOT_FIND);
					}
					const auto count = LegionMemberMap::get_legion_member_count(legion->get_legion_uuid());
					if(count >= limit)
					{
						return Response(Msg::ERR_LEGION_MEMBER_FULL);
					}
					const auto join_account = AccountMap::require(AccountUuid(req.account_uuid));
					// 处理审批结果
					if(req.bagree == 1)
					{
						// 同意加入,增加军团成员
						const auto utc_now = Poseidon::get_utc_time();

				//		legion->AddMember(AccountUuid(req.account_uuid),Data::Global::as_unsigned(Data::Global::SLOT_LEGION_MEMBER_DEFAULT_POWERID),utc_now,join_account->get_attribute(AccountAttributeIds::ID_DONATE),join_account->get_attribute(AccountAttributeIds::ID_WEEKDONATE),//  // //join_account->get_nick());

						legion->AddMember(join_account,Data::Global::as_unsigned(Data::Global::SLOT_LEGION_MEMBER_DEFAULT_POWERID),utc_now);

						// 如果还请求加入过其他军团，也需要做善后操作，删除其他所有加入请求
						LegionApplyJoinMap::deleteInfo(LegionUuid(member->get_legion_uuid()),AccountUuid(req.account_uuid),true);

						// 别人邀请加入的也要清空
						LegionInviteJoinMap::deleteInfo_by_invited_uuid(AccountUuid(req.account_uuid));

						bdeleteAll = true;
					}
					else
					{
						// 不同意加入
					}


					// 发邮件告诉结果
					if(req.bagree == 1)
						legion->sendmail(join_account,ChatMessageTypeIds::ID_LEVEL_LEGION_JOIN,legion->get_nick());
					else
					{
						legion->sendmail(join_account,ChatMessageTypeIds::ID_LEVEL_LEGION_REFUSE_APPLY,legion->get_nick() + ","+ account->get_nick());
					}


					/*
					const auto target_session = PlayerSessionMap::get(AccountUuid(req.account_uuid));
					if(target_session)
					{
						Msg::SC_LegionEmailMessage msg;
						msg.legion_uuid = LegionUuid(member->get_legion_uuid()).str();
						msg.legion_name = legion->get_nick();
						if(req.bagree == 1)
							msg.ntype = Legion::LEGION_EMAIL_TYPE::LEGION_EMAIL_JOIN;
						else
						{
							msg.ntype = Legion::LEGION_EMAIL_TYPE::LEGION_EMAIL_REFUSE_JOIN;
						}
						target_session->send(msg);
					}

					*/

				}
				else
				{
					return Response(Msg::ERR_LEGION_NO_POWER);
				}

				// 删除该条请求数据
				LegionApplyJoinMap::deleteInfo(LegionUuid(member->get_legion_uuid()),AccountUuid(req.account_uuid),bdeleteAll);

				return Response(Msg::ST_OK)  << req.account_uuid;
			}
			else
				return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND_DATA);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_LegionInviteJoinReqMessage, account, session, req)
{
	PROFILE_ME;

	// 查看邀请的玩家是否存在
	std::vector<boost::shared_ptr<Account>> accounts;
	AccountMap::get_by_nick(accounts,std::move(req.nick));

	if(accounts.empty())
	{
		// 按照昵称没找到 再按照uuid找一次
		try
		{
			const auto InviteMember = AccountMap::get(AccountUuid(req.nick));

			if(InviteMember)
			{
				// 查看对方是否已经加入军团
				const auto memberinfo = LegionMemberMap::get_by_account_uuid(InviteMember->get_account_uuid());
				if(memberinfo)
				{
					return Response(Msg::ERR_ACCOUNT_HAVE_LEGION) << req.nick;
				}
			}
			else
			{
				LOG_EMPERY_CENTER_DEBUG("CS_LegionInviteJoinReqMessage ==============返回没找到玩家： ",req.nick);
				return Response(Msg::ERR_LEGION_CANNOT_FIND_ACCOUNT);
			}
		}
		catch (const std::exception&)
		{
				LOG_EMPERY_CENTER_DEBUG("CS_LegionInviteJoinReqMessage ==============返回没找到玩家： ",req.nick);
				return Response(Msg::ERR_LEGION_CANNOT_FIND_ACCOUNT);
		}

	}

	const auto account_uuid = account->get_account_uuid();
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 查看军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			// 判断成员数
			const auto levelinfo = Data::LegionCorpsLevel::get(LegionCorpsLevelId(boost::lexical_cast<uint32_t>(legion->get_attribute(LegionAttributeIds::ID_LEVEL))));
			std::uint64_t limit = 0;
			if(levelinfo)
			{
				limit = levelinfo->legion_member_max;
			}
			else
			{
				return Response(Msg::ERR_LEGION_CONFIG_CANNOT_FIND);
			}
			const auto count = LegionMemberMap::get_legion_member_count(legion->get_legion_uuid());
			if(count >= limit)
			{
				return Response(Msg::ERR_LEGION_MEMBER_FULL);
			}


			// 查看member是否有权限
			const auto titileid = member->get_attribute(LegionMemberAttributeIds::ID_TITLEID);
			if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_INVITE))
			{
				// 有权限邀请
				const auto utc_now = Poseidon::get_utc_time();
				// 查看对方是否已经加入军团
				if(!accounts.empty())
				{
					for(auto it = accounts.begin(); it != accounts.end();++it)
					{
						const auto info = *it;
						const auto memberinfo = LegionMemberMap::get_by_account_uuid(info->get_account_uuid());
						if(memberinfo)
						{
							if(accounts.size() == 1)
							{
								return Response(Msg::ERR_ACCOUNT_HAVE_LEGION)  << req.nick;
							}
							else
							{
								continue;
							}
						}

						// 查看是否已经邀请过
						const auto& invite =  LegionInviteJoinMap::find(AccountUuid(account_uuid),LegionUuid(member->get_legion_uuid()),AccountUuid(info->get_account_uuid()));
						if(invite)
						{
							LOG_EMPERY_CENTER_DEBUG("CS_LegionInviteJoinReqMessage ==============已经邀请过 ",req.nick);

							return Response(Msg::ERR_LEGION_INVITED)  << req.nick;
						}
						else
						{
							const auto& primary_castle =  WorldMap::get_primary_castle(AccountUuid(info->get_account_uuid()));
							if(!primary_castle || primary_castle->get_level() < Data::Global::as_unsigned(Data::Global::SLOT_LEGION_MAINCITY_LOARD_LEVEL_LIMIT))
							{
								return Response(Msg::ERR_LEGION_MAINCITY_LOARD_LEVEL_LIMIT);
							}

							LOG_EMPERY_CENTER_DEBUG("CS_LegionInviteJoinReqMessage =============没邀请过,加入邀请列表 ",req.nick);
							// 添加到邀请列表中
							auto obj = boost::make_shared<MySql::Center_LegionInviteJoin>( LegionUuid(member->get_legion_uuid()).get(),account_uuid.get(),info->get_account_uuid().get(),utc_now);
							obj->enable_auto_saving();
							auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);

							LegionInviteJoinMap::insert(obj);

							LOG_EMPERY_CENTER_DEBUG("CS_LegionInviteJoinReqMessage apply_count = ",LegionInviteJoinMap::get_apply_count(account_uuid));

							// 如果玩家在线，可以直接发玩家
							const auto target_session = PlayerSessionMap::get(AccountUuid(info->get_account_uuid()));
							if(target_session)
							{
								// 临时把邀请列表发送给用户
								std::vector<boost::shared_ptr<MySql::Center_LegionInviteJoin>> invites;
								LegionInviteJoinMap::get_by_invited_uuid(invites, AccountUuid(info->get_account_uuid()));

								Msg::SC_LegionInviteList msg;
								msg.invites.reserve(invites.size());

								for(auto it = invites.begin(); it != invites.end(); ++it )
								{
									auto &elem = *msg.invites.emplace(msg.invites.end());
									auto infos = *it;

									elem.legion_uuid = LegionUuid(infos->get_legion_uuid()).str();
									const auto legion = LegionMap::get(LegionUuid(elem.legion_uuid));
									elem.legion_name = legion->get_nick();

									elem.icon = legion->get_attribute(LegionAttributeIds::ID_ICON);

									const auto leader_account = AccountMap::get(AccountUuid(legion->get_attribute(LegionAttributeIds::ID_LEADER)));

									elem.leader_name = leader_account->get_nick();

									elem.level = legion->get_attribute(LegionAttributeIds::ID_LEVEL);
									elem.membercount = boost::lexical_cast<std::string>(LegionMemberMap::get_legion_member_count(legion->get_legion_uuid()));
									elem.autojoin = legion->get_attribute(LegionAttributeIds::ID_AUTOJOIN);
									elem.rank = legion->get_attribute(LegionAttributeIds::ID_RANK);
									elem.notice = legion->get_attribute(LegionAttributeIds::ID_CONTENT);

								}

								LOG_EMPERY_CENTER_INFO("CS_GetLegionInviteListMessage size==============================================",msg.invites.size());

								target_session->send(msg);
							}

							return Response(Msg::ST_OK)  << req.nick;

						}
					}
				}
				else
				{
					// 按照昵称没找到 再按照uuid找一次
					const auto InviteMember = AccountMap::get(AccountUuid(req.nick));

					// 查看是否已经邀请过
					const auto invite =  LegionInviteJoinMap::find(AccountUuid(account_uuid),LegionUuid(member->get_legion_uuid()),AccountUuid(InviteMember->get_account_uuid()));
					if(!invite)
					{
						const auto& primary_castle =  WorldMap::get_primary_castle(AccountUuid(InviteMember->get_account_uuid()));
						if(!primary_castle || primary_castle->get_level() < Data::Global::as_unsigned(Data::Global::SLOT_LEGION_MAINCITY_LOARD_LEVEL_LIMIT))
						{
							return Response(Msg::ERR_LEGION_MAINCITY_LOARD_LEVEL_LIMIT);
						}

						LOG_EMPERY_CENTER_DEBUG("CS_LegionInviteJoinReqMessage 没邀请过,加入邀请列表 ",req.nick);
						// 添加到邀请列表中
						auto obj = boost::make_shared<MySql::Center_LegionInviteJoin>( LegionUuid(member->get_legion_uuid()).get(),account_uuid.get(),InviteMember->get_account_uuid().get(),utc_now);
						obj->enable_auto_saving();
						auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);

						LegionInviteJoinMap::insert(obj);

						LOG_EMPERY_CENTER_DEBUG("CS_LegionInviteJoinReqMessage apply_count = ",LegionInviteJoinMap::get_apply_count(account_uuid));

						// 如果玩家在线，可以直接发玩家
						const auto target_session = PlayerSessionMap::get(InviteMember->get_account_uuid());
						if(target_session)
						{
							// 临时把邀请列表发送给用户
							std::vector<boost::shared_ptr<MySql::Center_LegionInviteJoin>> invites;
							LegionInviteJoinMap::get_by_invited_uuid(invites, InviteMember->get_account_uuid());

							Msg::SC_LegionInviteList msg;
							msg.invites.reserve(invites.size());

							for(auto it = invites.begin(); it != invites.end(); ++it )
							{
								auto &elem = *msg.invites.emplace(msg.invites.end());
								auto infos = *it;

								elem.legion_uuid = LegionUuid(infos->get_legion_uuid()).str();
								const auto legion = LegionMap::get(LegionUuid(elem.legion_uuid));
								elem.legion_name = legion->get_nick();

								elem.icon = legion->get_attribute(LegionAttributeIds::ID_ICON);

								const auto leader_account = AccountMap::get(AccountUuid(legion->get_attribute(LegionAttributeIds::ID_LEADER)));

								elem.leader_name = leader_account->get_nick();

								elem.level = legion->get_attribute(LegionAttributeIds::ID_LEVEL);
								elem.membercount = boost::lexical_cast<std::string>(LegionMemberMap::get_legion_member_count(legion->get_legion_uuid()));
								elem.autojoin = legion->get_attribute(LegionAttributeIds::ID_AUTOJOIN);
								elem.rank = legion->get_attribute(LegionAttributeIds::ID_RANK);
								elem.notice = legion->get_attribute(LegionAttributeIds::ID_CONTENT);
							}

							LOG_EMPERY_CENTER_INFO("CS_GetLegionInviteListMessage size==============================================",msg.invites.size());

							target_session->send(msg);
						}
						return Response(Msg::ST_OK)  << req.nick;
					}
					else
					{
						LOG_EMPERY_CENTER_DEBUG("CS_LegionInviteJoinReqMessage 已经邀请过 ",req.nick);

						return Response(Msg::ERR_LEGION_INVITED)  << req.nick;
					}

				}
			}
			else
			{
				return Response(Msg::ERR_LEGION_NO_POWER);
			}

			return Response(Msg::ST_OK)  << req.nick;
		}
		else
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
	}
	else
	{
		// 自己没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();

}

PLAYER_SERVLET(Msg::CS_LegionInviteJoinResMessage, account, session, req)
{
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(!member)
	{
		// 查看军团是否存在
		const auto legion = LegionMap::get(LegionUuid(req.legion_uuid));
		if(legion)
		{
			// 是否有该邀请
			const auto invite =  LegionInviteJoinMap::find_inviteinfo_by_user(account_uuid,LegionUuid(req.legion_uuid));
			if(invite)
			{
				// 
				bool bdeleteAll = false;
				// 同意加入,增加军团成员
				const auto join_account = AccountMap::require(account_uuid);
				if(req.bagree)
				{
					const auto utc_now = Poseidon::get_utc_time();
				//	legion->AddMember(account_uuid,Data::Global::as_unsigned(Data::Global::SLOT_LEGION_MEMBER_DEFAULT_POWERID),utc_now,join_account->get_attribute(AccountAttributeIds::ID_DONATE),account->get_attribute(AccountAttributeIds::ID_WEEKDONATE),account->get_nick());

					legion->AddMember(join_account,Data::Global::as_unsigned(Data::Global::SLOT_LEGION_MEMBER_DEFAULT_POWERID),utc_now);

					// 如果还请求加入过其他军团，也需要做善后操作，删除其他所有加入请求
					LegionApplyJoinMap::deleteInfo(LegionUuid(req.legion_uuid),account_uuid,true);

					bdeleteAll = true;

					legion->sendmail(join_account,ChatMessageTypeIds::ID_LEVEL_LEGION_JOIN,legion->get_nick());
				}
				else
				{
					// 发邮件告诉拒绝加入
					legion->sendmail(join_account,ChatMessageTypeIds::ID_LEVEL_LEGION_REFUSE_INVITE, account->get_nick());
				}

				// 删除数据
				LegionInviteJoinMap::deleteInfo_by_invitedres_uuid(account_uuid,LegionUuid(req.legion_uuid),bdeleteAll);

				return Response(Msg::ST_OK);
			}
			else
			{
				return Response(Msg::ERR_LEGION_NOTFIND_INVITE_INFO);
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}

	}
	else
	{
		return Response(Msg::ERR_ACCOUNT_HAVE_LEGION);
	}

	return Response();
}


PLAYER_SERVLET(Msg::CS_QuitLegionReqMessage, account, session, req)
{
	PROFILE_ME;
	const auto account_uuid = account->get_account_uuid();

	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID))),Legion::LEGION_POWER::LEGION_POWER_QUIT))
			{
				return Response(Msg::ERR_LEGION_QUIT_ERROR_LEGIONLEADER);
			}
			else
			{
				// 是否已经在退会等待中
				if(member->get_attribute(LegionMemberAttributeIds::ID_QUITWAITTIME) == Poseidon::EMPTY_STRING)
				{
					// 设置退会等待时间
					boost::container::flat_map<LegionMemberAttributeId, std::string> Attributes;

					const auto utc_now = Poseidon::get_utc_time();

					std::string strtime = boost::lexical_cast<std::string>(utc_now + 60 * 1000 * Data::Global::as_unsigned(Data::Global::SLOT_LEGION_LEAVE_WAIT_MINUTE));  // 5分钟的默认退出等待时间

					strtime = strtime.substr(0,10);

					LOG_EMPERY_CENTER_INFO("CS_QuitLegionReqMessage ==============================================",utc_now,"   strtime:",strtime);

					Attributes[LegionMemberAttributeIds::ID_QUITWAITTIME] = strtime;

					member->set_attributes(std::move(Attributes));

					return Response(Msg::ST_OK);
					/* 真正退出军团逻辑
					const auto legion_uuid = member->get_legion_uuid();

					LegionMemberMap::deletemember(account_uuid);

					LOG_EMPERY_CENTER_INFO("CS_QuitLegionReqMessage members size==============================================",LegionMemberMap::get_legion_member_count(legion_uuid));
					*/
				}

				return Response(Msg::ST_OK);
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}

		return Response(Msg::ST_OK);
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CancelQuitLegionReqMessage, account, session, req)
{
	PROFILE_ME;
	const auto account_uuid = account->get_account_uuid();

	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID))),Legion::LEGION_POWER::LEGION_POWER_QUIT))
			{
				return Response(Msg::ERR_LEGION_QUIT_ERROR_LEGIONLEADER);
			}
			else
			{
				// 是否已经在退会等待中
				if(member->get_attribute(LegionMemberAttributeIds::ID_QUITWAITTIME) != Poseidon::EMPTY_STRING)
				{
					// 取消退会等待时间
					boost::container::flat_map<LegionMemberAttributeId, std::string> Attributes;

					Attributes[LegionMemberAttributeIds::ID_QUITWAITTIME] = "";

					member->set_attributes(std::move(Attributes));

					return Response(Msg::ST_OK);
				}

				return Response(Msg::ST_OK);
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}

		return Response(Msg::ST_OK);
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_LookLegionMembersReqMessage, account, session, req)
{
	PROFILE_ME;

	// 查找指定军团
	const auto legion = LegionMap::get(LegionUuid(req.legion_uuid));
	if(legion)
	{
		// 查找对应军团团员
		std::vector<boost::shared_ptr<LegionMember>> members;
		LegionMemberMap::get_by_legion_uuid(members, LegionUuid(req.legion_uuid));

		Msg::SC_LookLegionMembers msg;
		msg.members.reserve(members.size());
		for(auto it = members.begin(); it != members.end(); ++it )
		{
			auto &elem = *msg.members.emplace(msg.members.end());
			auto info = *it;

			elem.titleid = info->get_attribute(LegionMemberAttributeIds::ID_TITLEID);

			const auto legionmember = AccountMap::require(info->get_account_uuid());
			elem.nick = legionmember->get_nick();
			elem.icon = legionmember->get_attribute(AccountAttributeIds::ID_AVATAR);

		}

		session->send(msg);

		return Response(Msg::ST_OK);
	}
	else
	{
		return Response(Msg::ERR_LEGION_CANNOT_FIND);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_LegionMemberGradeReqMessage, account, session, req)
{
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	// 查看两个人是否属于同一军团
	if(!LegionMemberMap::is_in_same_legion(account_uuid,AccountUuid(req.account_uuid)))
	{
		return Response(Msg::ERR_LEGION_NOT_IN_SAME_LEGION);
	}

   // 判断自己军团是否存在
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			const auto target_account = AccountMap::get(AccountUuid(req.account_uuid));
			if(!target_account)
			{
				return Response(Msg::ERR_LEGION_CANNOT_FIND_ACCOUNT);
			}
			// 判断两个人是否属于同一个军团
			const auto othermember = LegionMemberMap::get_by_account_uuid(AccountUuid(req.account_uuid));
			if(othermember && othermember->get_legion_uuid() == member->get_legion_uuid())
			{
				// 查看自己是否拥有该权限
				if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID))),Legion::LEGION_POWER::LEGION_POWER_MEMBERGRADE))
				{
					return Response(Msg::ERR_LEGION_NO_POWER);
				}
				// 查看两者的权限
				const auto titleid= boost::lexical_cast<uint>(othermember->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
				auto own_titleid = boost::lexical_cast<uint>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
				if(titleid > own_titleid)
				{
					if(req.bup)  // 升级
					{
						// 查看目标是否有该权限
						if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(titleid),Legion::LEGION_POWER::LEGION_POWER_GRADEUP))
						{
							// 目标没有权限
							return Response(Msg::ERR_LEGION_TARGET_NO_POWER);
						}
						// 是否升级后和自己同级
						if(titleid -1 == own_titleid)
						{
							// 不能调整到和自己同等级
							return Response(Msg::ERR_LEGION_LEVEL_CANNOT_SAME);
						}
						// 再检查下，要升级的目标层级当前的人数
						std::vector<boost::shared_ptr<LegionMember>> members;
						LegionMemberMap::get_member_by_titleid(members, member->get_legion_uuid(),titleid - 1);
						LOG_EMPERY_CENTER_DEBUG("找到目标等级的人数 ========================================= ", members.size());
						if(members.size() < Data::LegionCorpsPower::get_member_max_num(LegionCorpsPowerId(titleid - 1)))
						{
							boost::container::flat_map<LegionMemberAttributeId, std::string> modifiers;
							modifiers[LegionMemberAttributeIds::ID_TITLEID] = boost::lexical_cast<std::string>(titleid -1);
							othermember->set_attributes(modifiers);
						}
						else
						{
							return Response(Msg::ERR_LEGION_LEVEL_MEMBERS_LIMIT);
						}
					}
					else // 降级
					{
						// 是否升级后和自己同级
						if(titleid +1 == own_titleid)
						{
							// 不能调整到和自己同等级
							return Response(Msg::ERR_LEGION_LEVEL_CANNOT_SAME);
						}

						// 查看目标是否有该权限
						if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(titleid),Legion::LEGION_POWER::LEGION_POWER_GRADEDOWN))
						{
							// 目标没有权限
							return Response(Msg::ERR_LEGION_TARGET_NO_POWER);
						}

						// 再检查下，要升级的目标层级当前的人数
						std::vector<boost::shared_ptr<LegionMember>> members;
						LegionMemberMap::get_member_by_titleid(members, member->get_legion_uuid(),titleid + 1);
						LOG_EMPERY_CENTER_DEBUG("找到目标等级的人数 ========================================= ", members.size());
						if(members.size() < Data::LegionCorpsPower::get_member_max_num(LegionCorpsPowerId(titleid + 1)))
						{
							boost::container::flat_map<LegionMemberAttributeId, std::string> modifiers;
							modifiers[LegionMemberAttributeIds::ID_TITLEID] = boost::lexical_cast<std::string>(titleid +1);
							othermember->set_attributes(modifiers);
						}
						else
						{
							return Response(Msg::ERR_LEGION_LEVEL_MEMBERS_LIMIT);
						}
					}

					// 广播给军团其他成员
					Msg::SC_LegionNoticeMsg msg;
					msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_GRADE;
					msg.nick = target_account->get_nick();
					if(req.bup)  // 升级
					{
						msg.ext1 = boost::lexical_cast<std::string>(titleid -1);
					}
					else
					{
						msg.ext1 = boost::lexical_cast<std::string>(titleid + 1);
					}
					legion->sendNoticeMsg(msg);

					return Response(Msg::ST_OK);
				}
				else
				{
					return Response(Msg::ERR_LEGION_NO_POWER);
				}
			}
			else
			{
				return Response(Msg::ERR_LEGION_NOT_IN_SAME_LEGION);
			}

		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_KickLegionMemberReqMessage, account, session, req)
{
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	// 是不是自己
	if(account_uuid == AccountUuid(req.account_uuid))
	{
		return Response(Msg::ERR_LEGION_TARGET_IS_OWN);
	}

	// 查看两个人是否属于同一军团
	if(!LegionMemberMap::is_in_same_legion(account_uuid,AccountUuid(req.account_uuid)))
	{
		return Response(Msg::ERR_LEGION_NOT_IN_SAME_LEGION);
	}

	// 判断自己军团是否存在
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			// 查看权限
			const auto othermember = LegionMemberMap::get_by_account_uuid(AccountUuid(req.account_uuid));
			const auto target_account = AccountMap::get(AccountUuid(req.account_uuid));
			if(othermember && target_account)
			{
				// 查看两者的权限
				if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID))),Legion::LEGION_POWER::LEGION_POWER_KICKMEMBER))
				{
					return Response(Msg::ERR_LEGION_NO_POWER);
				}
				const auto titleid= boost::lexical_cast<uint>(othermember->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
				if( titleid > boost::lexical_cast<uint>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID)))
				{
					const auto kicktime = othermember->get_attribute(LegionMemberAttributeIds::ID_KICKWAITTIME);
					if(req.bCancle)
					{
						// 检查是否存在踢出等待时间
						if(!kicktime.empty() || kicktime != Poseidon::EMPTY_STRING)
						{
							boost::container::flat_map<LegionMemberAttributeId, std::string> Attributes;
							Attributes[LegionMemberAttributeIds::ID_KICKWAITTIME] = "";

							othermember->set_attributes(Attributes);
						}
					}
					else
					{
						// 检查是否已经存在踢出等待时间
						if(!kicktime.empty() || kicktime != Poseidon::EMPTY_STRING)
						{
							return Response(Msg::ERR_LEGION_KICK_IN_WAITTIME);
						}
						const auto utc_now = Poseidon::get_utc_time();
						// 判断玩家最后一次在线时间

						const auto strlast_logout_time = target_account->get_attribute(AccountAttributeIds::ID_LAST_LOGGED_OUT_TIME);
						if(!strlast_logout_time.empty()  && strlast_logout_time != Poseidon::EMPTY_STRING)
						{
							const auto last_logout_time = boost::lexical_cast<std::uint64_t>(target_account->get_attribute(AccountAttributeIds::ID_LAST_LOGGED_OUT_TIME));
							if(utc_now - last_logout_time >= Data::Global::as_unsigned(Data::Global::SLOT_LEGION_KICK_OUTTIME) * 60 * 1000)
							{
								// 被踢出时发邮件
								legion->sendmail(target_account,ChatMessageTypeIds::ID_LEVEL_LEGION_KICK,legion->get_nick() + ","+ account->get_nick());

								// 广播给军团其他成员
								Msg::SC_LegionNoticeMsg msg;
								msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_KICK;
								msg.nick = target_account->get_nick();
								msg.ext1 = "";
								legion->sendNoticeMsg(msg);

								// 离线时间过长的可以直接踢掉
								LegionMemberMap::deletemember(AccountUuid(req.account_uuid),true);

								return Response(Msg::ST_OK);

							}
						}

						// 设置移除等待时间
						boost::container::flat_map<LegionMemberAttributeId, std::string> Attributes;

						std::string strtime = boost::lexical_cast<std::string>(utc_now + 60 * 1000 *  Data::Global::as_unsigned(Data::Global::SLOT_LEGION_LEAVE_WAIT_MINUTE));  // 5分钟的默认退出等待时间

						strtime = strtime.substr(0,10);

						LOG_EMPERY_CENTER_INFO("CS_KickLegionMemberReqMessage ==============================================",utc_now,"   strtime:",strtime);

						Attributes[LegionMemberAttributeIds::ID_KICKWAITTIME] = strtime;

						othermember->set_attributes(Attributes);


					}

					return Response(Msg::ST_OK);
				}
				else
				{
					return Response(Msg::ERR_LEGION_NO_POWER);
				}
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}


PLAYER_SERVLET(Msg::CS_AttornLegionReqMessage, account, session, req)
{
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	// 是不是自己
	if(account_uuid == AccountUuid(req.account_uuid))
	{
		return Response(Msg::ERR_LEGION_TARGET_IS_OWN);
	}
	// 查看两个人是否属于同一军团
	if(!LegionMemberMap::is_in_same_legion(account_uuid,AccountUuid(req.account_uuid)))
	{
		return Response(Msg::ERR_LEGION_NOT_IN_SAME_LEGION);
	}

	// 判断自己军团是否存在
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 自己是否是军团长
		if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID))),Legion::LEGION_POWER::LEGION_POWER_ATTORN))
		{
			// 检查军团是否存在
			const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
			if(legion)
			{
				// 是否已经处于转让等待中
				LOG_EMPERY_CENTER_INFO("CS_AttornLegionReqMessage  转让等待时间：==============================================",legion->get_attribute(LegionAttributeIds::ID_ATTORNTIME));
				if(legion->get_attribute(LegionAttributeIds::ID_ATTORNTIME) == Poseidon::EMPTY_STRING)
				{
					// 设置转让军团等待时间
					boost::container::flat_map<LegionAttributeId, std::string> Attributes;

					const auto utc_now = Poseidon::get_utc_time();

					std::string strtime = boost::lexical_cast<std::string>(utc_now + 60 * 1000 * Data::Global::as_unsigned(Data::Global::SLOT_LEGION_ATTORN_LEADER_WAITTIME));  // 5分钟的默认退出等待时间

					strtime = strtime.substr(0,10);

					LOG_EMPERY_CENTER_INFO("CS_AttornLegionReqMessage ==============================================",utc_now,"   strtime:",strtime);

					Attributes[LegionAttributeIds::ID_ATTORNTIME] = strtime;
					Attributes[LegionAttributeIds::ID_ATTORNLEADER] = req.account_uuid;

					legion->set_attributes(Attributes);

					return Response(Msg::ST_OK);

				}
				else
				{
					return Response(Msg::ERR_LEGION_ATTORN_IN_WAITTIME);
				}
			}
			else
			{
				return Response(Msg::ERR_LEGION_CANNOT_FIND);
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_NO_POWER);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CancleAttornLegionReqMessage, account, session, req)
{
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	// 判断自己军团是否存在
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 自己是否是军团长
		if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID))),Legion::LEGION_POWER::LEGION_POWER_ATTORN))
		{
			// 检查军团是否存在
			const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
			if(legion)
			{
				// 是否已经处于转让等待中
				if(legion->get_attribute(LegionAttributeIds::ID_ATTORNTIME) != Poseidon::EMPTY_STRING)
				{
					// 重置转让等待时间
					boost::container::flat_map<LegionAttributeId, std::string> Attributes;

					Attributes[LegionAttributeIds::ID_ATTORNTIME] = "";
					Attributes[LegionAttributeIds::ID_ATTORNLEADER] = "";

					legion->set_attributes(Attributes);

					return Response(Msg::ST_OK);
				}
				else
				{
					return Response(Msg::ERR_LEGION_NO_POWER);
				}
			}
			else
			{
				return Response(Msg::ERR_LEGION_CANNOT_FIND);
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_NO_POWER);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}


PLAYER_SERVLET(Msg::CS_disbandLegionReqMessage, account, session, req)
{
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	// 判断自己军团是否存在
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 军团成员数量是否为1
		if(LegionMemberMap::get_legion_member_count(LegionUuid(member->get_legion_uuid())) != 1)
		{
			return Response(Msg::ERR_LEGION_HAS_OTHER_MEMBERS);
		}
		// 自己是否是军团长
		if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID))),Legion::LEGION_POWER::LEGION_POWER_DISBAND))
		{
			// 需要发消息去联盟服务器，查看是不是盟主
			Msg::SL_disbandLegionReq msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

			LOG_EMPERY_CENTER_DEBUG("CS_disbandLegionReqMessage response: code =================== ", tresult.first);

			return Response(std::move(tresult.first));

			/*
			const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
			if(legion)
			{
				// 发邮件告诉结果
				legion->sendmail(account,ChatMessageTypeIds::ID_LEVEL_LEGION_DISBAND,legion->get_nick());

				// 解散军团
				LegionMap::deletelegion(LegionUuid(member->get_legion_uuid()));

				return Response(Msg::ST_OK);
			}

			*/
		}
		else
		{
			return Response(Msg::ERR_LEGION_NO_POWER);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_banChatLegionReqMessage, account, session, req)
{
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	// 是不是自己
	if(account_uuid == AccountUuid(req.account_uuid))
	{
		return Response(Msg::ERR_LEGION_TARGET_IS_OWN);
	}

	const auto target_account = AccountMap::get(AccountUuid(req.account_uuid));
	if(!target_account)
	{
		return Response(Msg::ERR_LEGION_CANNOT_FIND_ACCOUNT);
	}
	// 查看两个人是否属于同一军团
	if(!LegionMemberMap::is_in_same_legion(account_uuid,AccountUuid(req.account_uuid)))
	{
		return Response(Msg::ERR_LEGION_NOT_IN_SAME_LEGION);
	}

	// 判断自己是否加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			// 是否有权限设置
			const auto othermember = LegionMemberMap::get_by_account_uuid(AccountUuid(req.account_uuid));
			if(othermember)
			{
				// 查看两者的权限
				if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID))),Legion::LEGION_POWER::LEGION_POWER_CHAT))
				{
					return Response(Msg::ERR_LEGION_NO_POWER);
				}
				const auto titleid= boost::lexical_cast<uint>(othermember->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
				if( titleid > boost::lexical_cast<uint>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID)))
				{
					// 设置禁言标识
					boost::container::flat_map<LegionMemberAttributeId, std::string> Attributes;

					Attributes[LegionMemberAttributeIds::ID_SPEAKFLAG] = boost::lexical_cast<std::string>(req.bban);

					othermember->set_attributes(Attributes);

					// 广播给军团其他成员
					Msg::SC_LegionNoticeMsg msg;
					msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_SPEACK;
					msg.nick = target_account->get_nick();
					msg.ext1 = boost::lexical_cast<std::string>(req.bban);
					legion->sendNoticeMsg(msg);


					return Response(Msg::ST_OK);
				}
				else
				{
					return Response(Msg::ERR_LEGION_NO_POWER);
				}
			}
			else
			{
				return Response(Msg::ERR_LEGION_NOT_IN_SAME_LEGION);
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_LegionNoticeReqMessage, account, session, req)
{
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	// 判断自己是否加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			// 是否有权限设置
			if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID))),Legion::LEGION_POWER::LEGION_POWER_CHANGENOTICE))
			{
				// 设置军团公告
				boost::container::flat_map<LegionAttributeId, std::string> Attributes;

				Attributes[LegionAttributeIds::ID_CONTENT] = std::move(req.notice);

				legion->set_attributes(Attributes);

				// 广播给军团其他成员
				Msg::SC_LegionNoticeMsg msg;
				msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_CHANGENOTICE;
				msg.nick = account->get_nick();
				msg.ext1 = req.notice;
				legion->sendNoticeMsg(msg);


				return Response(Msg::ST_OK);
			}
			else
			{
				return Response(Msg::ERR_LEGION_NO_POWER);
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}


	return Response();
}

PLAYER_SERVLET(Msg::CS_GetLegionInviteListMessage, account, session, req)
{
	PROFILE_ME;
	// 临时把邀请列表发送给用户
	std::vector<boost::shared_ptr<MySql::Center_LegionInviteJoin>> invites;
	LegionInviteJoinMap::get_by_invited_uuid(invites, account->get_account_uuid());

	Msg::SC_LegionInviteList msg;
	msg.invites.reserve(invites.size());

	for(auto it = invites.begin(); it != invites.end(); ++it )
	{
		auto &elem = *msg.invites.emplace(msg.invites.end());
		auto info = *it;

		elem.legion_uuid = LegionUuid(info->get_legion_uuid()).str();
		const auto legion = LegionMap::get(LegionUuid(elem.legion_uuid));
		elem.legion_name = legion->get_nick();

		elem.icon = legion->get_attribute(LegionAttributeIds::ID_ICON);

		const auto leader_account = AccountMap::require(AccountUuid(legion->get_attribute(LegionAttributeIds::ID_LEADER)));

		elem.leader_name = leader_account->get_nick();
		elem.level = legion->get_attribute(LegionAttributeIds::ID_LEVEL);
		elem.membercount = boost::lexical_cast<std::string>(LegionMemberMap::get_legion_member_count(legion->get_legion_uuid()));
		elem.autojoin = legion->get_attribute(LegionAttributeIds::ID_AUTOJOIN);
		elem.rank = legion->get_attribute(LegionAttributeIds::ID_RANK);
		elem.notice = legion->get_attribute(LegionAttributeIds::ID_CONTENT);
	}

	LOG_EMPERY_CENTER_INFO("CS_GetLegionInviteListMessage size==============================================",msg.invites.size());

	session->send(msg);

	return Response(Msg::ST_OK);

}

PLAYER_SERVLET(Msg::CS_SearchLegionMessage, account, session, req)
{
	PROFILE_ME;

	LOG_EMPERY_CENTER_INFO("CS_SearchLegionMessage ************************ req.legion_name:",req.legion_name," req.leader_name:",req.leader_name," req.language:",req.language);
	const auto account_uuid = account->get_account_uuid();
	Msg::SC_SearchLegionRes msg;
	std::vector<boost::shared_ptr<Legion>> legions_results;
	if(!std::move(req.legion_name).empty())
	{

		std::vector<boost::shared_ptr<Legion>> legions;
		LegionMap::get_by_nick(legions, std::move(req.legion_name));
		if(!std::move(req.leader_name).empty())
		{
			// 如果同时有军团长昵称
			for(auto it = legions.begin(); it != legions.end(); ++it)
			{
				const auto leader =  AccountMap::require(AccountUuid((*it)->get_attribute(LegionAttributeIds::ID_LEADER)));
				if(leader->get_nick() == std::move(req.leader_name))
				{
					if(!std::move(req.language).empty())
					{
						if((*it)->get_attribute(LegionAttributeIds::ID_LANAGE) != std::move(req.language))
						continue;
					}
					legions_results.push_back(*it);
				}

				LOG_EMPERY_CENTER_INFO("CS_SearchLegionMessage 军团名称 军团长名称 找到************************ ",legions_results.size());
			}
		}
		else
		{
			legions_results = legions;

			LOG_EMPERY_CENTER_INFO("CS_SearchLegionMessage 只有军团名称 找到************************ ",legions_results.size());
		}
	}
	else if(!std::move(req.leader_name).empty())
	{
		// 按照军团长名称来查找
		std::vector<boost::shared_ptr<Account>> accounts;
		AccountMap::get_by_nick(accounts,std::move(req.leader_name));
		LOG_EMPERY_CENTER_INFO("CS_SearchLegionMessage 按照军团长名称玩家数量 找到************************ ",accounts.size());
		for(auto it = accounts.begin(); it != accounts.end(); ++it)
		{
			const  auto legion_member = LegionMemberMap::get_by_account_uuid((*it)->get_account_uuid());
			if(legion_member)
			{
				const auto legion = LegionMap::get(LegionUuid(legion_member->get_legion_uuid()));
				if(legion)
				{
					if(!std::move(req.language).empty())
					{
						if(legion->get_attribute(LegionAttributeIds::ID_LANAGE) != std::move(req.language))
						continue;
					}
					legions_results.push_back(legion);
				}
				LOG_EMPERY_CENTER_INFO("CS_SearchLegionMessage 按照军团长名称 找到************************ ",legions_results.size());
			}
		}
	}
	else
	{
		// 只按照语言来查找
		std::vector<boost::shared_ptr<Legion>> legions;
		LegionMap::get_all(legions, 0, 1000);


		for(auto it = legions.begin(); it != legions.end(); ++it )
		{
			if((*it)->get_attribute(LegionAttributeIds::ID_LANAGE) == std::move(req.language))
			{
				legions_results.push_back(*it);
			}
		}

		LOG_EMPERY_CENTER_INFO("CS_SearchLegionMessage 按照语言过滤后 找到************************ ",legions_results.size());
	}

	msg.legions.reserve(legions_results.size());

	for(auto it = legions_results.begin(); it != legions_results.end(); ++it )
	{
		auto &elem = *msg.legions.emplace(msg.legions.end());
		auto legion = *it;

		elem.legion_uuid = legion->get_legion_uuid().str();
		elem.legion_name = legion->get_nick();
		elem.legion_icon = legion->get_attribute(LegionAttributeIds::ID_ICON);

		const auto leader_account = AccountMap::require(AccountUuid(legion->get_attribute(LegionAttributeIds::ID_LEADER)));
		elem.legion_leadername = leader_account->get_nick();
		elem.level = legion->get_attribute(LegionAttributeIds::ID_LEVEL);

		elem.membercount = boost::lexical_cast<std::string>(LegionMemberMap::get_legion_member_count(legion->get_legion_uuid()));
		elem.autojoin = legion->get_attribute(LegionAttributeIds::ID_AUTOJOIN);
		elem.rank = legion->get_attribute(LegionAttributeIds::ID_RANK);
		elem.notice = legion->get_attribute(LegionAttributeIds::ID_CONTENT);

		const auto primary_castle =  WorldMap::get_primary_castle(leader_account->get_account_uuid());
		if(primary_castle)
		{
			elem.legion_Leader_maincity_loard_level =  boost::lexical_cast<std::string>(primary_castle->get_level());
		}
		else
		{
			elem.legion_Leader_maincity_loard_level =  "0";
		}
		elem.legion_create_time =  boost::lexical_cast<std::string>(legion->get_created_time());

		// 是否请求加入过
		const auto apply = LegionApplyJoinMap::find(account_uuid,legion->get_legion_uuid());
		if(apply)
		{
			elem.isapplyjoin = "1";
		}
		else
		{
			elem.isapplyjoin = "0";
		}

	}

	LOG_EMPERY_CENTER_INFO("CS_SearchLegionMessage size==============================================",msg.legions.size());

	session->send(msg);

	return Response(Msg::ST_OK);
}

PLAYER_SERVLET(Msg::CS_LegionChatMessage, account, session, req)
{
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	// 判断自己是否加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			if(member->get_attribute(LegionMemberAttributeIds::ID_SPEAKFLAG) == "1" )
			{
				return Response(Msg::ERR_LEGION_BAN_CHAT);
			}
			else
			{
				// 广播给军团其他成员
				Msg::SC_LegionNoticeMsg msg;
				msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_CHAT;
				msg.nick = account->get_nick();
				msg.ext1 = boost::lexical_cast<std::string>(req.content);
				legion->sendNoticeMsg(msg);

				return Response(Msg::ST_OK);
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_LegionDonateMessage, account, session, req)
{
	PROFILE_ME;

	LOG_EMPERY_CENTER_INFO("CS_LegionDonateMessage num==============================================",req.num);

	const auto account_uuid = account->get_account_uuid();

	// 判断自己是否加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			const auto &danate_info = Data::Global::as_object(Data::Global::SLOT_LEGION_DONATE_SCALE);

			for(auto it = danate_info.begin(); it != danate_info.end(); ++it)
			{
				const auto str = std::string(it->first.get());
		//		LOG_EMPERY_CENTER_INFO("CS_LegionDonateMessage  danate_info 解析==============================================",std::string(it->first.get()));

				if( str == boost::lexical_cast<std::string>(ItemIds::ID_DIAMONDS))
				{
					const auto  dvalue = static_cast<double>(danate_info.at(SharedNts::view(str.c_str())).get<double>());
					const auto lvalue = static_cast<double>(danate_info.at(SharedNts::view("5500001")).get<double>());
					const auto mvalue = static_cast<double>(danate_info.at(SharedNts::view("1103005")).get<double>());

		//			LOG_EMPERY_CENTER_INFO("CS_LegionDonateMessage  捐献钻石===========",dvalue," lvalue:",lvalue," mvalue:",mvalue);

					const auto item_box = ItemBoxMap::require(account_uuid);
					const auto  curDiamonds = item_box->get(ItemIds::ID_DIAMONDS).count;
					if(curDiamonds < req.num || req.num < dvalue / 100)
					{
						return Response(Msg::ERR_LEGION_DONATE_LACK);
					}
					else
					{
						std::vector<ItemTransactionElement> transaction;
						transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ItemIds::ID_DIAMONDS, req.num * 100,
								ReasonIds::ID_DOANTE_LEGION, 0, 0, req.num * 100);

						const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
						[&]{
							// 修改军团资金
							LOG_EMPERY_CENTER_ERROR("军团资金 捐献前  ===========",legion->get_attribute(LegionAttributeIds::ID_MONEY));
							boost::container::flat_map<LegionAttributeId, std::string> Attributes;
							const auto money = boost::lexical_cast<std::string>(std::ceil(boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY)) + req.num * 100 / dvalue * lvalue));
							Attributes[LegionAttributeIds::ID_MONEY] = money;
							LOG_EMPERY_CENTER_ERROR("军团资金 捐献后  ===========",money);
							legion->set_attributes(Attributes);

							const auto strweekdonate = member->get_attribute(LegionMemberAttributeIds::ID_WEEKDONATE);
							double weekdonate= 0;
							if(strweekdonate != Poseidon::EMPTY_STRING)
							{
								weekdonate = boost::lexical_cast<double>(strweekdonate);
							}

					//		LOG_EMPERY_CENTER_INFO("CS_LegionDonateMessage  周捐献数量 ===========",weekdonate);

							if(weekdonate < Data::Global::as_double(Data::Global::SLOT_LEGION_WEEK_DONATE_DIAMOND_LIMIT) / 100)
							{
								// 修改军团成员个人贡献
								LOG_EMPERY_CENTER_ERROR("CS_LegionDonateMessage  之前个人贡献 ===========",member->get_attribute(LegionMemberAttributeIds::ID_DONATE));

								boost::container::flat_map<LegionMemberAttributeId, std::string> Attributes1;

								auto strdotan = std::ceil(boost::lexical_cast<uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_DONATE)) + req.num * 100  / dvalue * mvalue);
								if(strdotan > Data::Global::as_double(Data::Global::SLOT_LEGION_WEEK_DONATE_DIAMOND_LIMIT) / 100)
									strdotan = Data::Global::as_double(Data::Global::SLOT_LEGION_WEEK_DONATE_DIAMOND_LIMIT) / 100;

								Attributes1[LegionMemberAttributeIds::ID_DONATE] = boost::lexical_cast<std::string>(strdotan);

								LOG_EMPERY_CENTER_ERROR("CS_LegionDonateMessage  捐献后个人贡献 ===========",strdotan);
				//				Attributes1[LegionMemberAttributeIds::ID_DONATE] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_DONATE)) + req.num / dvalue * mvalue);
								Attributes1[LegionMemberAttributeIds::ID_WEEKDONATE] = boost::lexical_cast<std::string>(weekdonate + req.num );

								LOG_EMPERY_CENTER_INFO("CS_LegionDonateMessage  捐献后个人周贡献 ===========",boost::lexical_cast<std::string>(weekdonate + req.num));
								member->set_attributes(std::move(Attributes1));
							}


							// 广播给军团其他成员
							Msg::SC_LegionNoticeMsg msg;
							msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_DONATE;
							msg.nick = account->get_nick();
							msg.ext1 = boost::lexical_cast<std::string>(req.num);
							legion->sendNoticeMsg(msg);

							//军团捐献任务
							try{
								Poseidon::enqueue_async_job([=]{
									PROFILE_ME;
									auto donate = req.num * 100 / dvalue * mvalue;
									const auto legion_uuid = LegionUuid(member->get_legion_uuid());
									const auto legion_task_box = LegionTaskBoxMap::require(legion_uuid);
									legion_task_box->check(TaskTypeIds::ID_LEGION_DONATE, TaskLegionKeyIds::ID_LEGION_DONATE,donate,account_uuid, 0, 0);
									legion_task_box->pump_status();

								});

							} catch (std::exception &e){
								LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
							}

							});

							if(insuff_item_id)
							{
								return Response(Msg::ERR_LEGION_CREATE_NOTENOUGH_MONEY);
							}
							return Response(Msg::ST_OK);
						}
				}
				else
				{
					return Response(Msg::ERR_LEGION_CONFIG_CANNOT_FIND);
				}
			}

		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}


PLAYER_SERVLET(Msg::CS_LegionExchangeItemMessage, account, session, req)
{
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	// 判断自己是否加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
				// 检查道具是否存在
				const auto &storeitem  = Data::LegionStore::get(LegionStoreGoodsId(req.goodsid));
				if(storeitem)
				{
					// 查看是否解锁
					for(auto it = storeitem->unlock_condition.begin(); it != storeitem->unlock_condition.end(); ++it)
					{
						if(it->first == "5500006")
						{
							// 取军团等级
							if(boost::lexical_cast<std::uint64_t>(legion->get_attribute(LegionAttributeIds::ID_LEVEL)) < it->second)
							{
								return Response(Msg::ERR_LEGION_STORE_LOCK);
							}
						}
						else
						{
							DEBUG_THROW(Exception, sslit("storeitem->unlock_condition unsupport"));
						}
					}
					boost::container::flat_map<LegionMemberAttributeId, std::string> Attributes;
					// 查看是否限购
					if(storeitem->limited_purchase != 0 )
					{
						if(req.num > storeitem->limited_purchase)
						{
							return Response(Msg::ERR_LEGION_STORE_LIMIT_PURCHASE);
						}
						const auto strgoodsid = boost::lexical_cast<std::string>(req.goodsid);
						const auto  recordinfo = member->get_attribute(LegionMemberAttributeIds::ID_LEGION_STORE_EXCHANGE_RECORD);

						LOG_EMPERY_CENTER_INFO("CS_LegionExchangeItemMessage  兑换记录 ===========",recordinfo);

						Poseidon::JsonObject  obj ;
						if(!recordinfo.empty() || recordinfo != Poseidon::EMPTY_STRING)
						{
							std::istringstream iss(recordinfo);
							auto val = Poseidon::JsonParser::parse_object(iss);
							obj = std::move(val);

							LOG_EMPERY_CENTER_INFO("CS_LegionExchangeItemMessage  兑换记录json解析 ===========",obj.dump());

							if(obj.find(SharedNts::view(strgoodsid.c_str())) != obj.end())
							{
								const auto  dvalue = static_cast<double>(obj.at(SharedNts::view(strgoodsid.c_str())).get<double>());

								LOG_EMPERY_CENTER_INFO("CS_LegionExchangeItemMessage  获得曾经的兑换次数 ===========",dvalue);
								if(dvalue < storeitem->limited_purchase)
								{
									obj[SharedNts(strgoodsid.c_str())] = dvalue + req.num;

									Attributes[LegionMemberAttributeIds::ID_LEGION_STORE_EXCHANGE_RECORD] = obj.dump();

									LOG_EMPERY_CENTER_INFO("CS_LegionExchangeItemMessage 记录限购内容 ===========",obj.dump());
								}
								else
								{
									LOG_EMPERY_CENTER_INFO("CS_LegionExchangeItemMessage  超过限购次数，不能限购 ===========",dvalue);
									return Response(Msg::ERR_LEGION_STORE_LIMIT_PURCHASE);
								}
							}
							else
							{
								obj[SharedNts(strgoodsid.c_str())] = 1;

								Attributes[LegionMemberAttributeIds::ID_LEGION_STORE_EXCHANGE_RECORD] = obj.dump();

								LOG_EMPERY_CENTER_INFO("CS_LegionExchangeItemMessage 空表直接记录记录限购内容 ===========",obj.dump());
							}

						}
						else
						{
							obj[SharedNts(strgoodsid.c_str())] = 1;

							Attributes[LegionMemberAttributeIds::ID_LEGION_STORE_EXCHANGE_RECORD] = obj.dump();

							LOG_EMPERY_CENTER_INFO("CS_LegionExchangeItemMessage 空表直接记录记录限购内容 ===========",obj.dump());
						}

					}

					// 查看需要消耗的资源是否够
					const auto trade_data = Data::ItemTrade::require(TradeId(storeitem->trading_id));
					for(auto it = trade_data->items_consumed.begin(); it != trade_data->items_consumed.end(); ++it)
					{
						if(boost::lexical_cast<std::string>(it->first.get()) == "1103005")
						{
							// 取个人贡献
							LOG_EMPERY_CENTER_INFO("CS_LegionExchangeItemMessage  个人贡献 ===========",boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_DONATE))," 价格：",it->second," 数量:",req.num);
							if(boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_DONATE)) < it->second * req.num)
							{
								return Response(Msg::ERR_LEGION_STORE_CONSUMED_LACK);
							}
							else
							{
								// 改变个人贡献
								Attributes[LegionMemberAttributeIds::ID_DONATE] = boost::lexical_cast<std::string>(boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_DONATE)) - it->second * req.num);
							}
						}
						else
						{
							DEBUG_THROW(Exception, sslit("storeitem->unlock_condition unsupport"));
						}
					}

					// 获得物品
					const auto item_box = ItemBoxMap::require(account_uuid);
					std::vector<ItemTransactionElement> transaction;

					for(auto it = trade_data->items_produced.begin(); it != trade_data->items_produced.end(); ++it)
					{
						transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemId(it->first), it->second,
							ReasonIds::ID_LEGION_STORE_EXCHANGE, 0, 0, it->second);
					}


					const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
					[&]{
						// 消耗处理
						member->set_attributes(std::move(Attributes));
					});

					if(insuff_item_id)
					{
						return Response(Msg::ERR_LEGION_STORE_CONSUMED_LACK);
					}

					return Response(Msg::ST_OK);
			}
			else
			{
				return Response(Msg::ERR_LEGION_STORE_CANNOT_FIND_ITEM);
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_LegionExchangeItemRecordMessage, account, session, req)
{
	PROFILE_ME;
	const auto account_uuid = account->get_account_uuid();

	// 判断自己是否加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
			const auto  recordinfo = member->get_attribute(LegionMemberAttributeIds::ID_LEGION_STORE_EXCHANGE_RECORD);

			Msg::SC_LegionStoreRecordInfo msg;
			msg.recordinfo = recordinfo;
			session->send(msg);

			return Response(Msg::ST_OK);
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_LegionContribution, account, session, req){
	PROFILE_ME;
	const auto account_uuid = account->get_account_uuid();

	// 判断自己是否加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion_uuid = LegionUuid(member->get_legion_uuid());
		const auto legion = LegionMap::get(legion_uuid);
		std::vector<boost::shared_ptr<LegionMember>> members;
		LegionMemberMap::get_by_legion_uuid(members,legion_uuid);
		if(legion)
		{
	        Msg::SC_LegionContributions msg;
			msg.legion_uuid = legion_uuid.str();
			auto legion_contribution_box = LegionTaskContributionBoxMap::get(legion_uuid);
			if(legion_contribution_box){
				msg.contributions.reserve(members.size());
				for(auto it = members.begin(); it != members.end(); ++it){
					auto &elem = *msg.contributions.emplace(msg.contributions.end());
					auto account_uuid = (*it)->get_account_uuid();
					LegionTaskContributionBox::TaskContributionInfo info = legion_contribution_box->get(account_uuid);
					elem.account_uuid = account_uuid.str();
					const auto temp_account = AccountMap::get(account_uuid);
					elem.account_nick = temp_account->get_nick();
					elem.day_contribution = info.day_contribution;
					elem.week_contribution = info.week_contribution;
					elem.total_contribution = info.total_contribution;
				}
			}
			LOG_EMPERY_CENTER_DEBUG("SC_LegionContributions:",msg);
			session->send(msg);

			return Response(Msg::ST_OK);
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}else{
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}
	return Response();
}

}