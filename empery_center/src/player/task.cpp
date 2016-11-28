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
						});
				});

	task_box->check_primary_tasks();
	task_box->check_daily_tasks_next(task_id);

	return Response();
}

}
