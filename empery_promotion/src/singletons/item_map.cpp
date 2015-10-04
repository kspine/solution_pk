#include "../precompiled.hpp"
#include "item_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../checked_arithmetic.hpp"
#include "../item_transaction_element.hpp"
#include "../mysql/item.hpp"
#include "../events/item.hpp"

namespace EmperyPromotion {

namespace {
	struct ItemElement {
		boost::shared_ptr<MySql::Promotion_Item> obj;

		AccountId accountId;
		ItemId itemId;
		std::pair<AccountId, ItemId> accountItem;

		explicit ItemElement(boost::shared_ptr<MySql::Promotion_Item> obj_)
			: obj(std::move(obj_))
			, accountId(obj->get_accountId()), itemId(obj->get_itemId())
			, accountItem(accountId, itemId)
		{
		}
	};

	MULTI_INDEX_MAP(ItemMapDelegator, ItemElement,
		MULTI_MEMBER_INDEX(accountId)
		MULTI_MEMBER_INDEX(itemId)
		UNIQUE_MEMBER_INDEX(accountItem)
	)

	boost::weak_ptr<ItemMapDelegator> g_itemMap;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::createConnection();

		const auto itemMap = boost::make_shared<ItemMapDelegator>();
		LOG_EMPERY_PROMOTION_INFO("Loading items...");
		conn->executeSql("SELECT * FROM `Promotion_Item`");
		while(conn->fetchRow()){
			auto obj = boost::make_shared<MySql::Promotion_Item>();
			obj->syncFetch(conn);
			obj->enableAutoSaving();
			itemMap->insert(ItemElement(std::move(obj)));
		}
		LOG_EMPERY_PROMOTION_INFO("Loaded ", itemMap->size(), " item(s).");
		g_itemMap = itemMap;
		handles.push(itemMap);
	}
}

boost::uint64_t ItemMap::getCount(AccountId accountId, ItemId itemId){
	PROFILE_ME;

	const auto itemMap = g_itemMap.lock();
	if(!itemMap){
		LOG_EMPERY_PROMOTION_WARNING("Item map is not loaded.");
		return 0;
	}

	const auto it = itemMap->find<2>(std::make_pair(accountId, itemId));
	if(it == itemMap->end<2>()){
		return 0;
	}
	return it->obj->get_count();
}
void ItemMap::getAllByAccountId(boost::container::flat_map<ItemId, boost::uint64_t> &ret, AccountId accountId){
	PROFILE_ME;

	const auto itemMap = g_itemMap.lock();
	if(!itemMap){
		LOG_EMPERY_PROMOTION_WARNING("Item map is not loaded.");
		return;
	}

	const auto range = itemMap->equalRange<0>(accountId);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret[it->itemId] = it->obj->get_count();
	}
}
void ItemMap::getAllByItemId(boost::container::flat_map<AccountId, boost::uint64_t> &ret, ItemId itemId){
	PROFILE_ME;

	const auto itemMap = g_itemMap.lock();
	if(!itemMap){
		LOG_EMPERY_PROMOTION_WARNING("Item map is not loaded.");
		return;
	}

	const auto range = itemMap->equalRange<1>(itemId);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret[it->accountId] = it->obj->get_count();
	}
}

namespace {
	struct TempResultElement {
		boost::shared_ptr<MySql::Promotion_Item> obj;
		AccountId accountId;

		boost::uint64_t oldCount;
		mutable boost::uint64_t newCount;

		explicit TempResultElement(boost::shared_ptr<MySql::Promotion_Item> obj_)
			: obj(std::move(obj_))
			, accountId(obj->get_accountId())
			, oldCount(obj->get_count()), newCount(oldCount)
		{
		}
	};

	MULTI_INDEX_MAP(TempResultMap, TempResultElement,
		UNIQUE_MEMBER_INDEX(obj)
		MULTI_MEMBER_INDEX(accountId)
	)
}

void ItemMap::commitTransaction(const ItemTransactionElement *elements, std::size_t count){
	PROFILE_ME;

	const auto insufficientItemId = commitTransactionNoThrow(elements, count);
	if(insufficientItemId){
		LOG_EMPERY_PROMOTION_ERROR("Item transaction failure: insufficientItemId = ", insufficientItemId);
		DEBUG_THROW(Exception, sslit("Item transaction failure"));
	}
}

ItemId ItemMap::commitTransactionNoThrow(const ItemTransactionElement *elements, std::size_t count){
	PROFILE_ME;

	if(count == 0){
		return ItemId(0);
	}

	const auto itemMap = g_itemMap.lock();
	if(!itemMap){
		LOG_EMPERY_PROMOTION_WARNING("Item map is not loaded.");
		DEBUG_THROW(Exception, sslit("Item map is not loaded."));
	}

	const auto localNow = Poseidon::getLocalTime();

	const auto withdrawn = boost::make_shared<bool>(true);
	TempResultMap tempResults;

	for(std::size_t i = 0; i < count; ++i){
		const auto accountId  = elements[i].m_accountId;
		const auto operation  = elements[i].m_operation;
		const auto itemId     = elements[i].m_itemId;
		const auto deltaCount = elements[i].m_deltaCount;

		auto &reason  = elements[i].m_reason;
		auto &param1  = elements[i].m_param1;
		auto &param2  = elements[i].m_param2;
		auto &param3  = elements[i].m_param3;
		auto &remarks = elements[i].m_remarks;

		if(deltaCount == 0){
			continue;
		}

		switch(operation){
		case ItemTransactionElement::OP_NONE:
			break;

		case ItemTransactionElement::OP_ADD:
			{
				auto itemIt = itemMap->find<2>(std::make_pair(accountId, itemId));
				if(itemIt == itemMap->end<2>()){
					auto obj = boost::make_shared<MySql::Promotion_Item>(accountId.get(), itemId.get(), 0, localNow, 0);
					obj->asyncSave(true);
					itemIt = itemMap->insert<2>(itemIt, ItemElement(std::move(obj)));
				}

				auto resultIt = tempResults.find<0>(itemIt->obj);
				if(resultIt == tempResults.end<0>()){
					resultIt = tempResults.insert<0>(resultIt, TempResultElement(itemIt->obj));
				}
				resultIt->newCount = checkedAdd(resultIt->newCount, deltaCount);

				LOG_EMPERY_PROMOTION_DEBUG("@ Item transaction: add: accountId = ", accountId, ", itemId = ", itemId,
					", deltaCount = ", deltaCount, ", oldCount = ", resultIt->oldCount, ", newCount = ", resultIt->newCount,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3, ", remarks = ", remarks);
				Poseidon::asyncRaiseEvent(
					boost::make_shared<Events::ItemChanged>(accountId, itemId, resultIt->oldCount, resultIt->newCount,
						static_cast<Events::ItemChanged::Reason>(reason), param1, param2, param3, std::move(remarks)),
					withdrawn);
			}
			break;

		case ItemTransactionElement::OP_REMOVE:
		case ItemTransactionElement::OP_REMOVE_SATURATED:
			{
				const auto itemIt = itemMap->find<2>(std::make_pair(accountId, itemId));
				if(itemIt == itemMap->end<2>()){
					if(operation != ItemTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_PROMOTION_DEBUG("Item not found: itemId = ", itemId);
						return itemId;
					}
					break;
				}
				auto resultIt = tempResults.find<0>(itemIt->obj);
				if(resultIt == tempResults.end<0>()){
					resultIt = tempResults.insert<0>(resultIt, TempResultElement(itemIt->obj));
				}
				if(resultIt->newCount >= deltaCount){
					resultIt->newCount -= deltaCount;
				} else {
					if(operation != ItemTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_PROMOTION_DEBUG("No enough items: accountId = ", accountId, ", itemId = ", itemId,
							", oldCount = ", resultIt->newCount, ", deltaCount = ", deltaCount);
						return itemId;
					}
					resultIt->newCount = 0;
				}
				LOG_EMPERY_PROMOTION_DEBUG("@ Item transaction: remove: accountId = ", accountId, ", itemId = ", itemId,
					", deltaCount = ", deltaCount, ", oldCount = ", resultIt->oldCount, ", newCount = ", resultIt->newCount,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3, ", remarks = ", remarks);
				Poseidon::asyncRaiseEvent(
					boost::make_shared<Events::ItemChanged>(accountId, itemId, resultIt->oldCount, resultIt->newCount,
						static_cast<Events::ItemChanged::Reason>(reason), param1, param2, param3, std::move(remarks)),
					withdrawn);
			}
			break;

		default:
			LOG_EMPERY_PROMOTION_ERROR("Unexpected operation type: ", static_cast<int>(operation));
			DEBUG_THROW(Exception, sslit("Unexpected operation type"));
		}
	}

	for(auto it = tempResults.begin<0>(); it != tempResults.end<0>(); ++it){
		it->obj->set_count(it->newCount); // noexcept
	}
	*withdrawn = false;

	return ItemId(0);
}

}
