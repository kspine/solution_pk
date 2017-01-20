#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/cs_task.hpp"
#include "../msg/err_task.hpp"
#include "../singletons/task_box_map.hpp"
#include "../task_box.hpp"
#include "../data/task.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"
#include "../legion_member.hpp"
#include "../legion_task_box.hpp"
#include "../singletons/legion_task_box_map.hpp"
#include "../singletons/legion_member_map.hpp"
#include "../legion_task_reward_box.hpp"
#include "../singletons/legion_task_reward_box_map.hpp"
#include "../source_ids.hpp"
#include "../legion_log.hpp"
#include "../msg/err_legion.hpp"
#include "../legion_member_attribute_ids.hpp"
#include "../msg/sc_legion.hpp"
#include "../singletons/legion_map.hpp"
#include "../legion_task_contribution_box.hpp"
#include "../singletons/legion_task_contribution_box_map.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_ItemGetAllTasks, account, session, /* req */){
	const auto task_box = TaskBoxMap::require(account->get_account_uuid());
	task_box->pump_status();

	task_box->synchronize_with_player(session);
	auto member = LegionMemberMap::get_by_account_uuid(account->get_account_uuid());
	if (member){
		const auto legion_uuid = LegionUuid(member->get_legion_uuid());
		const auto legion_task_box = LegionTaskBoxMap::require(legion_uuid);
		legion_task_box->pump_status();
		legion_task_box->synchronize_with_player(session);

		const auto legion_task_reward_box = LegionTaskRewardBoxMap::require(account->get_account_uuid());
		legion_task_reward_box->pump_status();
		legion_task_reward_box->synchronize_with_player(session);
	}
	return Response();
}

PLAYER_SERVLET(Msg::CS_ItemFetchTaskReward, account, session, req){
	const auto task_box = TaskBoxMap::require(account->get_account_uuid());
	task_box->pump_status();

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
	const auto castle = WorldMap::require_primary_castle(account->get_account_uuid());
	const auto utc_now = Poseidon::get_utc_time();

	const auto task_id = TaskId(req.task_id);
	const auto task_data = Data::TaskAbstract::require(task_id);

	auto info = task_box->get(task_id);
	if(info.expiry_time < utc_now){
		return Response(Msg::ERR_NO_SUCH_TASK) <<task_id;
	}
	if(info.rewarded){
		return Response(Msg::ERR_TASK_REWARD_FETCHED) <<task_id;
	}

	if(!task_box->has_been_accomplished(task_id)){
		return Response(Msg::ERR_TASK_NOT_ACCOMPLISHED) <<task_id;
	}

	std::vector<ItemTransactionElement> transaction;
	transaction.reserve(task_data->rewards.size());
	for(auto it = task_data->rewards.begin(); it != task_data->rewards.end(); ++it){
		transaction.emplace_back(ItemTransactionElement::OP_ADD, it->first, it->second,
			ReasonIds::ID_TASK_REWARD, task_id.get(), 0, 0);
	}
	item_box->commit_transaction(transaction, false,
                 [&]{
					std::vector<ResourceTransactionElement> transaction_res;
					transaction_res.reserve(task_data->rewards_resources.size());
					for(auto it = task_data->rewards_resources.begin(); it != task_data->rewards_resources.end(); ++it){
						transaction_res.emplace_back(ResourceTransactionElement::OP_ADD, it->first, it->second,
							ReasonIds::ID_TASK_REWARD, task_id.get(), 0, 0);
					}
					castle->commit_resource_transaction(transaction_res,
						[&]{
							info.rewarded = true;
							task_box->update(std::move(info));
						},SourceIds::ID_TASK_REWARD);
				},SourceIds::ID_TASK_REWARD);

	task_box->check_primary_tasks();
	task_box->check_daily_tasks_next(task_id);

	return Response();
}

PLAYER_SERVLET(Msg::CS_ItemFetchLegionTaskReward, account, session, req){
	PROFILE_ME;
	const auto account_uuid = account->get_account_uuid();
	const auto legion_uuid = LegionUuid(req.legion_uuid);
	const auto task_id     = TaskId(req.task_id);
	const auto stage       = req.stage;
	const auto legion_task_box = LegionTaskBoxMap::require(legion_uuid);
	legion_task_box->pump_status();
	const auto task_data = Data::TaskLegion::require(task_id);

	const auto utc_now = Poseidon::get_utc_time();
	auto info = legion_task_box->get(task_id);
	if(info.expiry_time < utc_now){
		return Response(Msg::ERR_NO_SUCH_TASK) << task_id;
	}
	if(!legion_task_box->has_been_stage_accomplished(task_id,stage)){
		return Response(Msg::ERR_TASK_NOT_STAGE_ACCOMPLISHED) << task_id;
	}

	const auto legion_member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(!legion_member){
		return Response(Msg::ERR_LEGION_NOT_JOIN) << account_uuid;
	}
	if(legion_member->get_legion_uuid() != legion_uuid){
		return Response(Msg::ERR_LEGION_NOT_IN_SAME_LEGION) << legion_uuid;
	}

	//发送个人奖励
	auto it_personal_reward = task_data->stage_personal_reward.find(stage);
	const auto &personal_rewards  = it_personal_reward->second;
	std::uint64_t personal_donate = 0;
	for(auto itl = personal_rewards.begin(); itl != personal_rewards.end(); ++itl){
		auto resource_id = itl->first;
		if(resource_id.get() == 1103005){
			personal_donate += itl->second;
		}
	}

	auto legion_task_reward_box = LegionTaskRewardBoxMap::require(account_uuid);
	LegionTaskRewardBox::TaskRewardInfo task_reward_info = legion_task_reward_box->get(task_data->type);
	if(task_reward_info.created_time != 0){
		auto &progress = task_reward_info.progress;
		auto pit = progress->find(stage);
		if((pit != progress->end()) && (pit->second > 0)){
			return Response(Msg::ERR_TASK_STAGE_REWARD_FETCHED) << task_id;;
		}
		task_reward_info.last_reward_time = utc_now;
		auto new_progress = boost::make_shared<LegionTaskRewardBox::Progress>(*progress);
		(*new_progress)[stage] = 1;
		task_reward_info.progress = new_progress;
		legion_task_reward_box->update(std::move(task_reward_info));
	}
	else{
		task_reward_info.created_time = utc_now;
		task_reward_info.last_reward_time = utc_now;
		auto progress = boost::make_shared<LegionTaskRewardBox::Progress>();
		progress->emplace(stage,1);
		task_reward_info.progress = progress;
		legion_task_reward_box->insert(std::move(task_reward_info));
	}

	if(personal_donate > 0){
		boost::container::flat_map<LegionMemberAttributeId, std::string> legion_attributes_modifer;
		std::string donate = legion_member->get_attribute(LegionMemberAttributeIds::ID_DONATE);
		if(donate.empty()){
			legion_attributes_modifer[LegionMemberAttributeIds::ID_DONATE] = boost::lexical_cast<std::string>(personal_donate);
			LegionLog::LegionPersonalDonateTrace(account_uuid,0,personal_donate,ReasonIds::ID_LEGION_TASK_STAGE_REWARD,task_id.get(),stage,0);
		}else{
			legion_attributes_modifer[LegionMemberAttributeIds::ID_DONATE] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(donate) + personal_donate);
			LegionLog::LegionPersonalDonateTrace(account_uuid,boost::lexical_cast<uint64_t>(donate),boost::lexical_cast<uint64_t>(donate) + personal_donate,ReasonIds::ID_LEGION_TASK_STAGE_REWARD,task_id.get(),stage,0);
		}
		legion_member->set_attributes(std::move(legion_attributes_modifer));
		auto legion_task_contribution_box = LegionTaskContributionBoxMap::require(legion_uuid);
		legion_task_contribution_box->update(account_uuid,personal_donate);
		// 广播通知
		const auto legion = LegionMap::require(legion_uuid);
		Msg::SC_LegionNoticeMsg msg;
		msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_TASK_CHANGE;
		msg.nick = "";
		msg.ext1 = "";
		legion->sendNoticeMsg(msg);
	}else{
		LOG_EMPERY_CENTER_WARNING("task stage award personal donate = 0, task_id ",task_id," stage = ",stage);
	}
	return Response();
}

}
