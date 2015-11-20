#include "../precompiled.hpp"
#include "item_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../item_transaction_element.hpp"
#include "../mysql/item.hpp"
#include "../events/item.hpp"

namespace EmperyPromotion {

namespace {
	struct ItemElement {
		boost::shared_ptr<MySql::Promotion_Item> obj;

		AccountId account_id;
		ItemId item_id;
		std::pair<AccountId, ItemId> account_item;

		explicit ItemElement(boost::shared_ptr<MySql::Promotion_Item> obj_)
			: obj(std::move(obj_))
			, account_id(obj->get_account_id()), item_id(obj->get_item_id())
			, account_item(account_id, item_id)
		{
		}
	};

	MULTI_INDEX_MAP(ItemMapDelegator, ItemElement,
		MULTI_MEMBER_INDEX(account_id)
		MULTI_MEMBER_INDEX(item_id)
		UNIQUE_MEMBER_INDEX(account_item)
	)

	boost::weak_ptr<ItemMapDelegator> g_item_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto item_map = boost::make_shared<ItemMapDelegator>();
		LOG_EMPERY_PROMOTION_INFO("Loading items...");
		conn->execute_sql("SELECT * FROM `Promotion_Item`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Promotion_Item>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			item_map->insert(ItemElement(std::move(obj)));
		}
		LOG_EMPERY_PROMOTION_INFO("Loaded ", item_map->size(), " item(s).");
		g_item_map = item_map;
		handles.push(item_map);
	}
}

boost::uint64_t ItemMap::get_count(AccountId account_id, ItemId item_id){
	PROFILE_ME;

	const auto item_map = g_item_map.lock();
	if(!item_map){
		LOG_EMPERY_PROMOTION_WARNING("Item map is not loaded.");
		return 0;
	}

	const auto it = item_map->find<2>(std::make_pair(account_id, item_id));
	if(it == item_map->end<2>()){
		return 0;
	}
	return it->obj->get_count();
}
void ItemMap::get_all_by_account_id(boost::container::flat_map<ItemId, boost::uint64_t> &ret, AccountId account_id){
	PROFILE_ME;

	const auto item_map = g_item_map.lock();
	if(!item_map){
		LOG_EMPERY_PROMOTION_WARNING("Item map is not loaded.");
		return;
	}

	const auto range = item_map->equal_range<0>(account_id);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret[it->item_id] = it->obj->get_count();
	}
}
void ItemMap::get_all_by_item_id(boost::container::flat_map<AccountId, boost::uint64_t> &ret, ItemId item_id){
	PROFILE_ME;

	const auto item_map = g_item_map.lock();
	if(!item_map){
		LOG_EMPERY_PROMOTION_WARNING("Item map is not loaded.");
		return;
	}

	const auto range = item_map->equal_range<1>(item_id);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret[it->account_id] = it->obj->get_count();
	}
}

namespace {
	struct TempResultElement {
		boost::shared_ptr<MySql::Promotion_Item> obj;
		AccountId account_id;

		boost::uint64_t old_count;
		mutable boost::uint64_t new_count;

		explicit TempResultElement(boost::shared_ptr<MySql::Promotion_Item> obj_)
			: obj(std::move(obj_))
			, account_id(obj->get_account_id())
			, old_count(obj->get_count()), new_count(old_count)
		{
		}
	};

	MULTI_INDEX_MAP(TempResultMap, TempResultElement,
		UNIQUE_MEMBER_INDEX(obj)
		MULTI_MEMBER_INDEX(account_id)
	)
}

void ItemMap::commit_transaction(const ItemTransactionElement *elements, std::size_t count){
	PROFILE_ME;

	const auto insufficient_item_id = commit_transaction_nothrow(elements, count);
	if(insufficient_item_id){
		LOG_EMPERY_PROMOTION_ERROR("Item transaction failure: insufficient_item_id = ", insufficient_item_id);
		DEBUG_THROW(Exception, sslit("Item transaction failure"));
	}
}

ItemId ItemMap::commit_transaction_nothrow(const ItemTransactionElement *elements, std::size_t count){
	PROFILE_ME;

	if(count == 0){
		return ItemId(0);
	}

	const auto item_map = g_item_map.lock();
	if(!item_map){
		LOG_EMPERY_PROMOTION_WARNING("Item map is not loaded.");
		DEBUG_THROW(Exception, sslit("Item map is not loaded."));
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto withdrawn = boost::make_shared<bool>(true);
	TempResultMap temp_results;

	for(std::size_t i = 0; i < count; ++i){
		const auto account_id  = elements[i].m_account_id;
		const auto operation  = elements[i].m_operation;
		const auto item_id     = elements[i].m_item_id;
		const auto delta_count = elements[i].m_delta_count;

		auto &reason  = elements[i].m_reason;
		auto &param1  = elements[i].m_param1;
		auto &param2  = elements[i].m_param2;
		auto &param3  = elements[i].m_param3;
		auto &remarks = elements[i].m_remarks;

		if(delta_count == 0){
			continue;
		}

		switch(operation){
		case ItemTransactionElement::OP_NONE:
			break;

		case ItemTransactionElement::OP_ADD:
			{
				auto item_it = item_map->find<2>(std::make_pair(account_id, item_id));
				if(item_it == item_map->end<2>()){
					auto obj = boost::make_shared<MySql::Promotion_Item>(account_id.get(), item_id.get(), 0, utc_now, 0);
					obj->async_save(true);
					item_it = item_map->insert<2>(item_it, ItemElement(std::move(obj)));
				}

				auto result_it = temp_results.find<0>(item_it->obj);
				if(result_it == temp_results.end<0>()){
					result_it = temp_results.insert<0>(result_it, TempResultElement(item_it->obj));
				}
				const auto old_count = result_it->new_count;
				result_it->new_count = checked_add(old_count, delta_count);

				LOG_EMPERY_PROMOTION_DEBUG("@ Item transaction: add: account_id = ", account_id, ", item_id = ", item_id,
					", delta_count = ", delta_count, ", old_count = ", result_it->old_count, ", new_count = ", result_it->new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3, ", remarks = ", remarks);
				Poseidon::async_raise_event(
					boost::make_shared<Events::ItemChanged>(account_id, item_id, old_count, result_it->new_count,
						static_cast<Events::ItemChanged::Reason>(reason), param1, param2, param3, std::move(remarks)),
					withdrawn);
			}
			break;

		case ItemTransactionElement::OP_REMOVE:
		case ItemTransactionElement::OP_REMOVE_SATURATED:
			{
				const auto item_it = item_map->find<2>(std::make_pair(account_id, item_id));
				if(item_it == item_map->end<2>()){
					if(operation != ItemTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_PROMOTION_DEBUG("Item not found: item_id = ", item_id);
						return item_id;
					}
					break;
				}
				auto result_it = temp_results.find<0>(item_it->obj);
				if(result_it == temp_results.end<0>()){
					result_it = temp_results.insert<0>(result_it, TempResultElement(item_it->obj));
				}
				const auto old_count = result_it->new_count;
				if(result_it->new_count >= delta_count){
					result_it->new_count -= delta_count;
				} else {
					if(operation != ItemTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_PROMOTION_DEBUG("No enough items: account_id = ", account_id, ", item_id = ", item_id,
							", old_count = ", old_count, ", delta_count = ", delta_count);
						return item_id;
					}
					result_it->new_count = 0;
				}
				LOG_EMPERY_PROMOTION_DEBUG("@ Item transaction: remove: account_id = ", account_id, ", item_id = ", item_id,
					", delta_count = ", delta_count, ", old_count = ", old_count, ", new_count = ", result_it->new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3, ", remarks = ", remarks);
				Poseidon::async_raise_event(
					boost::make_shared<Events::ItemChanged>(account_id, item_id, old_count, result_it->new_count,
						static_cast<Events::ItemChanged::Reason>(reason), param1, param2, param3, std::move(remarks)),
					withdrawn);
			}
			break;

		default:
			LOG_EMPERY_PROMOTION_ERROR("Unexpected operation type: ", static_cast<int>(operation));
			DEBUG_THROW(Exception, sslit("Unexpected operation type"));
		}
	}

	for(auto it = temp_results.begin<0>(); it != temp_results.end<0>(); ++it){
		it->obj->set_count(it->new_count); // noexcept
	}
	*withdrawn = false;

	return ItemId(0);
}

}
