#ifndef EMPERY_CENTER_RESOURCE_UTILITIES_HPP_
#define EMPERY_CENTER_RESOURCE_UTILITIES_HPP_

#include <poseidon/exception.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "transaction_element.hpp"
#include "castle.hpp"
#include "item_box.hpp"
#include "task_box.hpp"
#include "log.hpp"
#include "checked_arithmetic.hpp"
#include "data/item.hpp"
#include "task_type_ids.hpp"

namespace EmperyCenter {

template<typename CallbackT>
std::pair<ResourceId, ItemId> try_decrement_resources(const boost::shared_ptr<Castle> &castle,
	const boost::shared_ptr<ItemBox> &item_box, const boost::shared_ptr<TaskBox> &task_box,
	const boost::container::flat_map<ResourceId, std::uint64_t> &resources_to_consume,
	const boost::container::flat_map<ItemId, std::uint64_t> &tokens_to_consume,
	ReasonId reason, std::uint64_t param1, std::uint64_t param2, std::uint64_t param3, CallbackT &&callback)
{
	PROFILE_ME;

	boost::container::flat_map<ResourceId, std::uint64_t> resources_consumed;
	resources_consumed = resources_to_consume; // resources_consumed.reserve(resources_to_consume.size());
	boost::container::flat_map<ItemId, std::uint64_t> tokens_consumed;
	tokens_consumed.reserve(tokens_to_consume.size());

	std::vector<ResourceTransactionElement> resource_transaction;
	resource_transaction.reserve(resources_to_consume.size());
	std::vector<ItemTransactionElement> item_transaction;
	item_transaction.reserve(tokens_to_consume.size());

	for(auto it = tokens_to_consume.begin(); it != tokens_to_consume.end(); ++it){
		const auto item_id = it->first;
		const auto item_data = Data::Item::require(item_id);
		if(item_data->type.first != Data::Item::CAT_RESOURCE_TOKEN){
			LOG_EMPERY_CENTER_WARNING("The specified item is not a resource token: item_id = ", item_id);
			DEBUG_THROW(Exception, sslit("The specified item is not a resource token"));
		}
		const auto resource_id = ResourceId(item_data->type.second);
		const auto cit = resources_consumed.find(resource_id);
		if(cit == resources_consumed.end()){
			LOG_EMPERY_CENTER_DEBUG("$$ Unneeded token: item_id = ", item_id);
			continue;
		}
		const auto max_count_needed = checked_add(cit->second, item_data->value - 1) / item_data->value;
		LOG_EMPERY_CENTER_DEBUG("$$ Use token: item_id = ", item_id, ", count = ", it->second, ", max_count_needed = ", max_count_needed);
		const auto count_consumed = std::min(it->second, max_count_needed);
		cit->second = saturated_sub(cit->second, item_data->value * count_consumed);
		tokens_consumed.emplace(item_id, count_consumed);
	}

	for(auto it = resources_consumed.begin(); it != resources_consumed.end(); ++it){
		resource_transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, it->first, it->second,
			reason, param1, param2, param3);
	}
	for(auto it = tokens_consumed.begin(); it != tokens_consumed.end(); ++it){
		item_transaction.emplace_back(ItemTransactionElement::OP_REMOVE, it->first, it->second,
			reason, param1, param2, param3);
	}

	try {
		const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(resource_transaction,
			[&]{
				const auto insuff_item_id = item_box->commit_transaction_nothrow(item_transaction, true,
					[&]{
						std::forward<CallbackT>(callback)();
					});
				if(insuff_item_id){
					throw insuff_item_id;
				}
		});
		if(insuff_resource_id){
			throw insuff_resource_id;
		}
	} catch(ResourceId &insuff_resource_id){
		return std::make_pair(insuff_resource_id, ItemId());
	} catch(ItemId &insuff_item_id){
		return std::make_pair(ResourceId(), insuff_item_id);
	}

	try {
		for(auto it = resources_to_consume.begin(); it != resources_to_consume.end(); ++it){
			if(it->second == 0){
				continue;
			}
			task_box->check(TaskTypeIds::ID_CONSUME_RESOURCES, it->first.get(), it->second,
				castle, 0, 0);
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}

	return { };
}

}

#endif
