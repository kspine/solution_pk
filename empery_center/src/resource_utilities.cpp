#include "precompiled.hpp"
#include <poseidon/exception.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"
#include "transaction_element.hpp"
#include "castle.hpp"
#include "task_box.hpp"
#include "log.hpp"
#include "checked_arithmetic.hpp"
#include "task_type_ids.hpp"
#include "data/castle.hpp"

namespace EmperyCenter {

ResourceId try_decrement_resources(const boost::shared_ptr<Castle> &castle, const boost::shared_ptr<TaskBox> &task_box,
	const boost::container::flat_map<ResourceId, std::uint64_t> &resources_to_consume,
	ReasonId reason, std::uint64_t param1, std::uint64_t param2, std::uint64_t param3, const boost::function<void ()> &callback)
{
	PROFILE_ME;

	std::vector<ResourceTransactionElement> transaction;
	transaction.reserve(resources_to_consume.size());
	for(auto it = resources_to_consume.begin(); it != resources_to_consume.end(); ++it){
		const auto resource_id = it->first;
		const auto resource_to_consume = it->second;

		const auto resource_data = Data::CastleResource::require(resource_id);
		const auto token_id = resource_data->resource_token_id;
		std::uint64_t token_consumed = 0;
		if(token_id){
			token_consumed = std::min(resource_to_consume, castle->get_resource(token_id).amount);
		}
		const auto resource_consumed = checked_sub(resource_to_consume, token_consumed);
		if(token_consumed != 0){
			transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, token_id, token_consumed,
				reason, param1, param2, param3);
		}
		if(resource_consumed != 0){
			transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, resource_id, resource_consumed,
				reason, param1, param2, param3);
		}
	}
	const auto insuff_item_id = castle->commit_resource_transaction_nothrow(transaction, callback);
	if(insuff_item_id){
		return insuff_item_id;
	}

	try {
		for(auto it = resources_to_consume.begin(); it != resources_to_consume.end(); ++it){
			if(it->second == 0){
				continue;
			}
			task_box->check(TaskBox::CAT_NULL,TaskTypeIds::ID_CONSUME_RESOURCES, it->first.get(), it->second,
				castle, 0, 0);
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}

	return { };
}

}
