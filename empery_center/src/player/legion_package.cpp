#include "../precompiled.hpp"
#include "common.hpp"

#include "../msg/cs_legion_package.hpp"
#include "../msg/sc_legion_package.hpp"

#include "../msg/err_legion.hpp"
#include "../msg/err_legion_package.hpp"
#include "../msg/err_task.hpp"

#include "../legion.hpp"
#include "../legion_member.hpp"

#include "../legion_attribute_ids.hpp"
#include "../legion_member_attribute_ids.hpp"

#include "../singletons/item_box_map.hpp"
#include "../singletons/legion_map.hpp"
#include "../singletons/legion_member_map.hpp"
#include "../singletons/task_box_map.hpp"
#include "../singletons/world_map.hpp"

#include "../singletons/legion_package_pick_share_map.hpp"
#include "../singletons/legion_package_share_map.hpp"

#include "../item_box.hpp"
#include "../item_ids.hpp"
#include "../reason_ids.hpp"
#include "../task_box.hpp"
#include "../transaction_element.hpp"
#include "../castle.hpp"

#include "../data/global.hpp"
#include "../data/legion_package_corps_box.hpp"
#include "../data/task.hpp"
#include "../data/legion_corps_level.hpp"

#include "../mysql/legion.hpp"

#include "../cluster/utilsplit.hpp"

#include <poseidon/singletons/mysql_daemon.hpp>

namespace EmperyCenter
{
	PLAYER_SERVLET(Msg::CS_GetSharePackageInfoReqMessage, account, session, req)
	{
		PROFILE_ME;

		const auto account_uuid = account->get_account_uuid();

		//判决流程

		const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
		if (!member)
		{
			// LOG_EMPERY_CENTER_ERROR("未加入军团联盟 ERR_LEGION_NOT_JOIN;");

			//未加入军团联盟
			return Response(Msg::ERR_LEGION_NOT_JOIN);
		}

		const auto legion_uuid = LegionUuid(member->get_legion_uuid());
		const auto legion = LegionMap::get(legion_uuid);
		if (!legion)
		{
			// LOG_EMPERY_CENTER_ERROR("军团不存在 ERR_LEGION_CANNOT_FIND;");

			//军团不存在
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}

		// 回包消息流程

		// 回复分享信息
		const auto item_box = ItemBoxMap::require(account_uuid);

		std::vector<boost::shared_ptr<MySql::Center_Legion_Package_Share>> shares;
		LegionPackageShareMap::get_by_legion_uuid(shares, member->get_legion_uuid());

		Msg::SC_GetSharePackageInfoReqMessage msg;

		msg.share_package_taken_counts =
			item_box->get(ItemIds::ID_LEGION_PACKAGE_SHARE_COUNTS).count;

		//msg.share_package_array.reserve(shares.size());

		auto str_share_uuid = member->get_attribute(
			LegionMemberAttributeIds::ID_LEGION_PACKAGE_EXPIRE_FILTER);

		LOG_EMPERY_CENTER_ERROR("str_share_uuid", str_share_uuid);

		std::vector<std::string> share_uuidVec =
			UTIL_SPLIT::UtilSplit::Split(str_share_uuid, ",");

		static std::uint64_t count = 0;

		// 上限限制处理
		//auto share_count =
		//	item_box->get(ItemIds::ID_LEGION_PACKAGE_SHARE_COUNTS).count;

		//const auto task_box = TaskBoxMap::require(account_uuid);
		//const auto legion_package_task_num = task_box->get_package_task_num();

		//const auto levelinfo =
		//	Data::LegionCorpsLevel::require(LegionCorpsLevelId(boost::lexical_cast<uint32_t>
		//	(legion->get_attribute(LegionAttributeIds::ID_LEVEL))));

		//const auto legion_member_max = levelinfo->legion_member_max;
		//const auto legion_share_package_pick_limit = (legion_member_max - 1) * legion_package_task_num;

		//LOG_EMPERY_CENTER_FATAL("CS_GetSharePackageInfoReqMessage:legion_package_task_num", legion_package_task_num);
		//LOG_EMPERY_CENTER_FATAL("CS_GetSharePackageInfoReqMessage:legion_member_max", legion_member_max);
		//LOG_EMPERY_CENTER_FATAL("CS_GetSharePackageInfoReqMessage:legion_share_package_pick_limit", legion_share_package_pick_limit);
		//LOG_EMPERY_CENTER_FATAL("CS_GetSharePackageInfoReqMessage:share_count", share_count);

		for (auto it = shares.begin(); it != shares.end(); ++it)
		{
			/*auto &elem =
				*msg.share_package_array.emplace(msg.share_package_array.end());*/
			auto info = (*it);

			std::vector<std::string>::iterator it_find =
				std::find(share_uuidVec.begin(), share_uuidVec.end(),
					info->unlocked_get_share_uuid().to_string());
			if (it_find != share_uuidVec.end())
			{
				//LOG_EMPERY_CENTER_ERROR("Find Delete Expire Mark ;");
				continue;
			}

			//LOG_EMPERY_CENTER_ERROR("Join Legion Time:", member->get_created_time(), "share_package_time:", info->unlocked_get_share_package_time());

			if (member->get_created_time() > info->unlocked_get_share_package_time())
			{
				//LOG_EMPERY_CENTER_ERROR("Share After Join Legion ;");
				continue;
			}

			count++;

			auto &elem =
				*msg.share_package_array.emplace(msg.share_package_array.end());

			elem.share_uuid = info->unlocked_get_share_uuid().to_string();
			elem.account_uuid = info->unlocked_get_account_uuid().to_string();
			elem.task_id = info->unlocked_get_task_id();
			elem.task_package_item_id = info->unlocked_get_share_package_item_id();
			elem.share_package_item_id = info->unlocked_get_share_package_item_id();
			elem.share_package_time = info->unlocked_get_share_package_time();
		}

		msg.share_package_array.reserve(count);

		//LOG_EMPERY_CENTER_ERROR("msg.share_package_array;",msg.share_package_array.size());

		//LOG_EMPERY_CENTER_ERROR("msg.count", count);

		session->send(msg);

		count = 0;

		// 回复分享礼包领取信息
		std::vector<boost::shared_ptr<MySql::Center_Legion_Package_Pick_Share>>
			picks_share;
		LegionPackagePickShareMap::get_by_share_uuid(picks_share, account_uuid);

		Msg::SC_GetPickSharePackageInfoReqMessage pick_share_package_msg;

		pick_share_package_msg.pick_share_package_array.reserve(picks_share.size());

		for (auto it = picks_share.begin(); it != picks_share.end(); ++it)
		{
			auto &elem = *pick_share_package_msg.pick_share_package_array.emplace(
				pick_share_package_msg.pick_share_package_array.end());
			auto info = (*it);

			elem.share_uuid = info->unlocked_get_share_uuid().to_string();
			elem.account_uuid = info->unlocked_get_account_uuid().to_string();
			elem.share_package_status = info->unlocked_get_share_package_status();
		}

		session->send(pick_share_package_msg);

		return Response(Msg::ST_OK);
	}

	PLAYER_SERVLET(Msg::CS_ReceiveTaskRewardReqMessage, account, session, req)
	{
		PROFILE_ME;

		// 协议参数解析流程
		const auto task_id = req.task_id;
		const auto task_package_item_id = req.task_package_item_id;
		const auto share_package_item_id = req.share_package_item_id;

		const auto account_uuid = account->get_account_uuid();

		const auto item_box = ItemBoxMap::require(account_uuid);

		//判决流程

		const auto task_box = TaskBoxMap::require(account_uuid);
		if (!task_box->has_been_accomplished(TaskId(task_id)))
		{
			// LOG_EMPERY_CENTER_ERROR("指定任务未完成 ERR_TASK_NOT_ACCOMPLISHED;");

			//指定任务未完成
			return Response(Msg::ERR_TASK_NOT_ACCOMPLISHED) << task_id;
		}

		const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
		if (!member)
		{
			// LOG_EMPERY_CENTER_ERROR("未加入军团联盟 ERR_LEGION_NOT_JOIN;");

			//未加入军团联盟
			return Response(Msg::ERR_LEGION_NOT_JOIN);
		}

		const auto legion_uuid = member->get_legion_uuid();
		const auto legion = LegionMap::get(LegionUuid(legion_uuid));
		if (!legion)
		{
			// LOG_EMPERY_CENTER_ERROR("军团不存在 ERR_LEGION_CANNOT_FIND;");

			//军团不存在
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}

		const auto legion_level = legion->get_attribute(LegionAttributeIds::ID_LEVEL);
		bool checkTaskPackage =
			Data::LegionPackageCorpsBox::check_legion_task_package(
				TaskId(task_id), boost::lexical_cast<uint64_t>(task_package_item_id),
				boost::lexical_cast<uint64_t>(legion_level));

		if (!checkTaskPackage)
		{
			// LOG_EMPERY_CENTER_ERROR("军团任务礼包不存在
			// ERR_LEGION_PACKAGE_NOT_EXISTS;");

			//军团任务礼包不存在
			return Response(Msg::ERR_LEGION_PACKAGE_NOT_EXISTS);
		}

		bool checksharePackage =
			Data::LegionPackageCorpsBox::check_legion_share_package(
				TaskId(task_id), boost::lexical_cast<uint64_t>(share_package_item_id),
				boost::lexical_cast<uint64_t>(legion_level));
		if (!checksharePackage)
		{
			// LOG_EMPERY_CENTER_ERROR("军团分享礼包不存在
			// ERR_LEGION_PACKAGE_NOT_EXISTS;");

			//军团分享礼包不存在
			return Response(Msg::ERR_LEGION_PACKAGE_NOT_EXISTS);
		}

		//领取并分享流程
		//bool check_task_package_status = LegionPackageShareMap::check(account_uuid, TaskId(task_id), task_package_item_id);
		bool check_task_package_status = task_box->check_reward_status(TaskId(task_id));
		if (check_task_package_status)
		{
			// LOG_EMPERY_CENTER_ERROR("军团任务礼包已领取
			// ERR_LEGION_PACKAGE_TASK_RECEIVED;");

			//军团任务礼包已领取
			return Response(Msg::ERR_LEGION_PACKAGE_TASK_RECEIVED);
		}

		//领取任务礼包-入背包
		std::vector<ItemTransactionElement> transaction;

		transaction.emplace_back(
			ItemTransactionElement::OP_ADD, ItemId(task_package_item_id), 1,
			ReasonIds::ID_LEGION_PACKAGE_TASK_PACKAGE_ITEM, 0, 0, 0);

		// LOG_EMPERY_CENTER_ERROR("transaction 领取任务礼包-入背包");

		item_box->commit_transaction(transaction, false);

		// LOG_EMPERY_CENTER_ERROR("commit_transaction 领取任务礼包-入背包");

		//军团礼包存储系统接口【Center_Legion_Package_Pick】

		auto share_expire_inteval_sec = Data::Global::as_unsigned(
			Data::Global::SLOT_LEGION_PACKAGE_SHARE_EXPIRE_MINUTE);
		auto share_expire_inteval_sec2ms =
			checked_mul<std::uint64_t>(share_expire_inteval_sec, 60000);

		auto share_package_time = Poseidon::get_utc_time();
		auto share_package_expire_time =
			share_package_time + share_expire_inteval_sec2ms;

		// LOG_EMPERY_CENTER_ERROR("CS_ReceiveTaskRewardReqMessage
		// =============分享者领取礼包流程{记录军团礼包领取记录:结束} ");

		//分享礼包流程{}

		//军团礼包存储系统接口【Center_Legion_Package_Share】

		//分享信息存储
		// LOG_EMPERY_CENTER_ERROR("CS_ReceiveTaskRewardReqMessage
		// =============分享者分享礼包流程{记录军团礼包分享记录:开始} ");

		auto share_uuid = LegionPackageShareUuid(Poseidon::Uuid::random());

		auto obj_share = boost::make_shared<MySql::Center_Legion_Package_Share>(
			share_uuid.get(), member->get_legion_uuid().get(), account_uuid.get(),
			task_id, task_package_item_id, share_package_item_id, share_package_time,
			share_package_expire_time);

		obj_share->enable_auto_saving();

		Poseidon::MySqlDaemon::enqueue_for_saving(obj_share, false, true);

		LegionPackageShareMap::insert(obj_share);

		// 修正礼包任务领取状态
		task_box->update_reward_status(TaskId(task_id));

		// LOG_EMPERY_CENTER_ERROR("CS_ReceiveTaskRewardReqMessage
		// =============分享者分享礼包流程{记录军团礼包分享记录:结束} ");

		//客户端分享同步消息
		// LOG_EMPERY_CENTER_ERROR("CS_ReceiveTaskRewardReqMessage
		// =============客户端分享同步消息:开始 ");

		std::vector<boost::shared_ptr<LegionMember>> members;
		LegionMemberMap::get_by_legion_uuid(members, member->get_legion_uuid());
		if (!members.empty())
		{
			// LOG_EMPERY_CENTER_ERROR("客户端分享同步消息;");

			Msg::SC_ShareSyncMessage msg;
			msg.share_uuid = boost::lexical_cast<std::string>(share_uuid);
			msg.account_uuid = boost::lexical_cast<std::string>(account_uuid);
			msg.task_id = task_id;
			msg.share_package_item_id = share_package_item_id;
			msg.share_package_time = share_package_time;

			for (auto it = members.begin(); it != members.end(); ++it)
			{
				const auto target_session =
					PlayerSessionMap::get(AccountUuid((*it)->get_account_uuid()));
				if (NULL != target_session && target_session != session)
				{
					// LOG_EMPERY_CENTER_ERROR("客户端分享同步消息 内部;");
					target_session->send(msg);
				}
			}
		}
		// LOG_EMPERY_CENTER_ERROR("CS_ReceiveTaskRewardReqMessage
		// =============客户端分享同步消息:结束 ");

		//回复：领取并分享结果
		return Response(Msg::ST_OK);
	}

	PLAYER_SERVLET(Msg::CS_ReceiveSharedRewardReqMessage, account, session, req)
	{
		PROFILE_ME;

		// 协议参数解析流程

		auto unique_share_uuid = LegionPackageShareUuid(req.share_uuid);

		auto task_id = LegionPackageShareMap::get_task_id(unique_share_uuid);

		auto share_package_item_id =
			LegionPackageShareMap::get_share_package_item_id(unique_share_uuid);

		auto unique_account_uuid = account->get_account_uuid();
		auto item_box = ItemBoxMap::require(unique_account_uuid);

		auto share_count =
			item_box->get(ItemIds::ID_LEGION_PACKAGE_SHARE_COUNTS).count;

		//判决流程

		AccountUuid unique_share_account_uuid;
		LegionPackageShareMap::get_account_uuid(unique_share_uuid,
			unique_share_account_uuid);

		auto unique_task_id = TaskId(task_id);

		const auto member = LegionMemberMap::get_by_account_uuid(unique_account_uuid);
		if (!member)
		{
			// LOG_EMPERY_CENTER_ERROR("自己未加入军团联盟 ERR_LEGION_NOT_JOIN;");

			//自己未加入军团联盟
			return Response(Msg::ERR_LEGION_NOT_JOIN);
		}

		const auto unique_legion_uuid = member->get_legion_uuid();
		const auto legion = LegionMap::get(unique_legion_uuid);
		if (!legion)
		{
			// LOG_EMPERY_CENTER_ERROR("军团不存在 ERR_LEGION_CANNOT_FIND;");

			//军团不存在
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}

		const auto sender_member =
			LegionMemberMap::get_by_account_uuid(unique_share_account_uuid);
		if (sender_member)
		{
			if (!LegionMemberMap::is_in_same_legion(unique_account_uuid,
				unique_share_account_uuid))
			{
				// LOG_EMPERY_CENTER_ERROR("不在同一军团 ERR_LEGION_NOT_IN_SAME_LEGION;");

				// 不在同一军团
				return Response(Msg::ERR_LEGION_NOT_IN_SAME_LEGION);
			}
		}

		const auto legion_level = legion->get_attribute(LegionAttributeIds::ID_LEVEL);
		bool checkSharePackage =
			Data::LegionPackageCorpsBox::check_legion_share_package(
				unique_task_id, boost::lexical_cast<uint64_t>(share_package_item_id),
				boost::lexical_cast<uint64_t>(legion_level));

		if (!checkSharePackage)
		{
			// LOG_EMPERY_CENTER_ERROR("军团礼包不存在 ERR_LEGION_PACKAGE_NOT_EXISTS;");

			//军团礼包不存在
			return Response(Msg::ERR_LEGION_PACKAGE_NOT_EXISTS);
		}

		const auto share_package_expire_time =
			LegionPackageShareMap::get_share_package_expire_time(unique_share_uuid);
		if (share_package_expire_time < Poseidon::get_utc_time())
		{
			// LOG_EMPERY_CENTER_ERROR("军团礼包分享超期
			// ERR_LEGION_PACKAGE_SHARE_EXPIRE;");

			//军团礼包分享超期
			return Response(Msg::ERR_LEGION_PACKAGE_SHARE_EXPIRE);
		}

		//领取分享礼包流程
		bool check_share_package_status =
			LegionPackagePickShareMap::check_share_package_status(
				unique_share_uuid, unique_account_uuid);
		if (check_share_package_status)
		{
			// LOG_EMPERY_CENTER_ERROR("军团分享礼包已领取
			// ERR_LEGION_PACKAGE_SHARE_RECEIVED;");

			//军团分享礼包已领取
			return Response(Msg::ERR_LEGION_PACKAGE_SHARE_RECEIVED);
		}

		//分享礼包的每日上限=（军团当前人数上限-1）*单人任务数量。
		const auto task_box = TaskBoxMap::require(unique_account_uuid);
		const auto levelinfo =
			Data::LegionCorpsLevel::require(LegionCorpsLevelId(boost::lexical_cast<uint32_t>
			(legion->get_attribute(LegionAttributeIds::ID_LEVEL))));

		const auto legion_member_max = levelinfo->legion_member_max;

		const auto primary_castle = WorldMap::get_primary_castle(unique_account_uuid);
		if (!primary_castle)
		{
			return Response(Msg::ST_OK);
		}
		const unsigned legion_package_task_num = Data::TaskDaily::get_legion_package_task_num(primary_castle->get_level());

		const unsigned legion_share_package_pick_limit = (legion_member_max - 1) * legion_package_task_num;

		LOG_EMPERY_CENTER_FATAL("legion_package_task_num", legion_package_task_num);
		LOG_EMPERY_CENTER_FATAL("legion_member_max", legion_member_max);
		LOG_EMPERY_CENTER_FATAL("legion_share_package_pick_limit", legion_share_package_pick_limit);
		LOG_EMPERY_CENTER_FATAL("share_count", share_count);

		if (share_count >= legion_share_package_pick_limit)
		{
			// LOG_EMPERY_CENTER_ERROR("当日可领分享礼包次数不足
			// ERR_DAILY_SHARE_NUMBER_INSUFFICIENT;");

			//当日可领分享礼包次数不足
			return Response(Msg::ERR_DAILY_SHARE_NUMBER_INSUFFICIENT);
		}

		// 分享礼包已领次数道具累加
		//领取分享礼包-入背包
		std::vector<ItemTransactionElement> transaction;

		transaction.emplace_back(
			ItemTransactionElement::OP_ADD, ItemIds::ID_LEGION_PACKAGE_SHARE_COUNTS,
			1, ReasonIds::ID_LEGION_PACKAGE_SHARE_PACKAGE_COUNT_ITEM,
			account->get_promotion_level(), 0, 0);

		transaction.emplace_back(ItemTransactionElement::OP_ADD,
			ItemId(share_package_item_id), 1,
			ReasonIds::ID_LEGION_PACKAGE_SHARE_PACKAGE_ITEM,
			account->get_promotion_level(), 0, 0);

		item_box->commit_transaction(transaction, false);

		//军团礼包存储系统接口【Center_Legion_Package_Pick】

		//领取信息存储
		// LOG_EMPERY_CENTER_DEBUG("CS_ReceiveSharedRewardReqMessage
		// =============接收者领取分享礼包流程{记录军团分享礼包领取记录:开始} ");
		auto obj = boost::make_shared<MySql::Center_Legion_Package_Pick_Share>(
			unique_share_uuid.get(), unique_account_uuid.get(),
			(uint64_t)EPickShareStatus_Received);

		obj->enable_auto_saving();

		Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);

		LegionPackagePickShareMap::insert(obj);

		// LOG_EMPERY_CENTER_DEBUG("CS_ReceiveSharedRewardReqMessage
		// =============接收者领取分享礼包流程{记录军团分享礼包领取记录:结束} ");

		//回复：领取分享礼包结果
		return Response(Msg::ST_OK);
	}

	PLAYER_SERVLET(Msg::CS_DeleteExpirePackageMessage, account, session, req)
	{
		PROFILE_ME;

		auto unique_share_uuid = LegionPackageShareUuid(req.share_uuid);

		LOG_EMPERY_CENTER_ERROR("Msg::CS_DeleteExpirePackageMessage unique_share_uuid", unique_share_uuid);

		auto unique_account_uuid = account->get_account_uuid();
		const auto member = LegionMemberMap::get_by_account_uuid(unique_account_uuid);
		if (!member)
		{
			// LOG_EMPERY_CENTER_ERROR("自己未加入军团联盟 ERR_LEGION_NOT_JOIN;");

			//自己未加入军团联盟
			return Response(Msg::ERR_LEGION_NOT_JOIN);
		}

		const auto unique_legion_uuid = member->get_legion_uuid();
		const auto legion = LegionMap::get(unique_legion_uuid);
		if (!legion)
		{
			// LOG_EMPERY_CENTER_ERROR("军团不存在 ERR_LEGION_CANNOT_FIND;");

			//军团不存在
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}

		//领取分享礼包流程
		bool check_share_package_status =
			LegionPackagePickShareMap::check_share_package_status(
				unique_share_uuid, unique_account_uuid);
		if (check_share_package_status)
		{
			// LOG_EMPERY_CENTER_ERROR("军团分享礼包已领取
			// ERR_LEGION_PACKAGE_SHARE_RECEIVED;");

			//军团分享礼包已领取
			return Response(Msg::ERR_LEGION_PACKAGE_SHARE_RECEIVED);
		}

		const auto share_package_expire_time =
			LegionPackageShareMap::get_share_package_expire_time(unique_share_uuid);
		if (share_package_expire_time >= Poseidon::get_utc_time())
		{
			// LOG_EMPERY_CENTER_ERROR("军团礼包分享未超期
			// ERR_LEGION_PACKAGE_SHARE_EXPIRE;");

			//军团礼包分享未超期
			return Response(Msg::ERR_LEGION_PACKAGE_SHARE_NOTEXPIRE);
		}

		// 未领取超期处理

		auto str_share_uuid = member->get_attribute(
			LegionMemberAttributeIds::ID_LEGION_PACKAGE_EXPIRE_FILTER);

		std::vector<std::string> share_uuidVec = UTIL_SPLIT::UtilSplit::Split(str_share_uuid, ",");

		std::vector<std::string>::iterator it_find =
			std::find(share_uuidVec.begin(), share_uuidVec.end(), boost::lexical_cast<std::string>(unique_share_uuid));

		if (it_find == share_uuidVec.end())
		{
			str_share_uuid += ",";
			str_share_uuid += boost::lexical_cast<std::string>(unique_share_uuid);

			boost::container::flat_map<LegionMemberAttributeId, std::string> Attr;
			Attr[LegionMemberAttributeIds::ID_LEGION_PACKAGE_EXPIRE_FILTER] = str_share_uuid;

			member->set_attributes(Attr);

			//LOG_EMPERY_CENTER_ERROR("CS_DeleteExpirePackageMessage:str_unique_share_uuid", str_share_uuid);
		}

		return Response(Msg::ST_OK);
	}

	PLAYER_SERVLET(Msg::CS_OneKeyDeleteExpirePackageMessage, account, session, req)
	{
		PROFILE_ME;

		auto unique_account_uuid = account->get_account_uuid();
		auto member = LegionMemberMap::get_by_account_uuid(unique_account_uuid);
		if (!member)
		{
			// LOG_EMPERY_CENTER_ERROR("自己未加入军团联盟 ERR_LEGION_NOT_JOIN;");

			//自己未加入军团联盟
			return Response(Msg::ERR_LEGION_NOT_JOIN);
		}

		const auto unique_legion_uuid = member->get_legion_uuid();
		const auto legion = LegionMap::get(unique_legion_uuid);
		if (!legion)
		{
			// LOG_EMPERY_CENTER_ERROR("军团不存在 ERR_LEGION_CANNOT_FIND;");

			//军团不存在
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}

		// 未领取超期处理
		auto str_share_uuid = member->get_attribute(LegionMemberAttributeIds::ID_LEGION_PACKAGE_EXPIRE_FILTER);

		std::vector<std::string> share_uuidVec = UTIL_SPLIT::UtilSplit::Split(str_share_uuid, ",");

		auto utc_time = Poseidon::get_utc_time();

		std::vector<boost::shared_ptr<MySql::Center_Legion_Package_Share>> shares;
		LegionPackageShareMap::get_by_legion_uuid(shares, member->get_legion_uuid());
		for (auto it = shares.begin(); it != shares.end(); ++it)
		{
			auto info = (*it);
			if (info->unlocked_get_share_package_expire_time() < utc_time)
			{
				std::vector<std::string>::iterator it_find =
					std::find(share_uuidVec.begin(), share_uuidVec.end(), info->unlocked_get_share_uuid().to_string());

				if (it_find == share_uuidVec.end())
				{
					str_share_uuid += ",";
					str_share_uuid += boost::lexical_cast<std::string>(info->unlocked_get_share_uuid());

					//LOG_EMPERY_CENTER_ERROR("CS_OneKeyDeleteExpirePackageMessage:str_share_uuid", str_share_uuid);
				}
			}
		}

		boost::container::flat_map<LegionMemberAttributeId, std::string> Attr;
		Attr[LegionMemberAttributeIds::ID_LEGION_PACKAGE_EXPIRE_FILTER] = str_share_uuid;
		member->set_attributes(Attr);

		return Response(Msg::ST_OK);
	}
}