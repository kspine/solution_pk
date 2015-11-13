#include "precompiled.hpp"
#include "item_box.hpp"
#include "item_transaction_element.hpp"
#include "msg/sc_item.hpp"
#include "mysql/item.hpp"
#include "singletons/player_session_map.hpp"
#include "events/item.hpp"
#include "player_session.hpp"
#include "events/item.hpp"
#include "checked_arithmetic.hpp"
#include "data/item.hpp"
#include "reason_ids.hpp"

namespace EmperyCenter {

namespace {
	void fillItemInfo(ItemBox::ItemInfo &info, const boost::shared_ptr<MySql::Center_Item> &obj){
		PROFILE_ME;

		info.itemId = ItemId(obj->get_itemId());
		info.count  = obj->get_count();
	}

	void synchronizeItemWithClient(const AccountUuid &accountUuid, const boost::shared_ptr<MySql::Center_Item> &obj){
		PROFILE_ME;

		const auto session = PlayerSessionMap::get(accountUuid);
		if(session){
			try {
				Msg::SC_ItemChanged msg;
				msg.itemId = obj->get_itemId();
				msg.count  = obj->get_count();
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->forceShutdown();
			}
		}
	}
}

ItemBox::ItemBox(const AccountUuid &accountUuid)
	: m_accountUuid(accountUuid)
{
}
ItemBox::ItemBox(const AccountUuid &accountUuid,
	const std::vector<boost::shared_ptr<MySql::Center_Item>> &items)
	: ItemBox(accountUuid)
{
	for(auto it = items.begin(); it != items.end(); ++it){
		const auto &obj = *it;
		m_items.emplace(ItemId(obj->get_itemId()), obj);
	}
}
ItemBox::~ItemBox(){
}

void ItemBox::pumpStatus(bool forceSynchronizationWithClient){
	PROFILE_ME;

	const auto utcNow = Poseidon::getUtcTime();

	LOG_EMPERY_CENTER_DEBUG("Checking for init items: accountUuid = ", getAccountUuid());
	std::vector<ItemTransactionElement> transaction;
	std::vector<boost::shared_ptr<const Data::Item>> itemsToCheck;
	Data::Item::getInit(itemsToCheck);
	for(auto dit = itemsToCheck.begin(); dit != itemsToCheck.end(); ++dit){
		const auto &itemData = *dit;
		const auto it = m_items.find(itemData->itemId);
		if(it == m_items.end()){
			LOG_EMPERY_CENTER_DEBUG("> Adding items: itemId = ", itemData->itemId, ", initCount = ", itemData->initCount);
			transaction.emplace_back(ItemTransactionElement::OP_ADD, itemData->itemId, itemData->initCount,
				ReasonIds::ID_INIT_ITEMS, itemData->initCount, 0, 0);
		}
	}
	commitTransaction(transaction.data(), transaction.size());

	LOG_EMPERY_CENTER_DEBUG("Checking for auto increment items: accountUuid = ", getAccountUuid());
	transaction.clear();
	itemsToCheck.clear();
	Data::Item::getAutoInc(itemsToCheck);
	boost::container::flat_map<boost::shared_ptr<MySql::Center_Item>, boost::uint64_t> newTimestamps;
	for(auto dit = itemsToCheck.begin(); dit != itemsToCheck.end(); ++dit){
		const auto &itemData = *dit;
		auto it = m_items.find(itemData->itemId);
		if(it == m_items.end()){
			auto obj = boost::make_shared<MySql::Center_Item>(getAccountUuid().get(), itemData->itemId.get(), 0, 0);
			obj->asyncSave(true);
			it = m_items.emplace(itemData->itemId, std::move(obj)).first;
		}
		const auto &obj = it->second;

		boost::uint64_t autoIncPeriod, autoIncOffset;
		switch(itemData->autoIncType){
		case Data::Item::AIT_HOURLY:
			autoIncPeriod = 3600 * 1000;
			autoIncOffset = itemData->autoIncOffset;
			break;
		case Data::Item::AIT_DAILY:
			autoIncPeriod = 24 * 3600 * 1000;
			autoIncOffset = itemData->autoIncOffset;
			break;
		case Data::Item::AIT_WEEKLY:
			autoIncPeriod = 7 * 24 * 3600 * 1000;
			autoIncOffset = itemData->autoIncOffset + 3 * 24 * 3600 * 1000; // 注意 1970-01-01 是星期四。
			break;
		case Data::Item::AIT_PERIODIC:
			autoIncPeriod = itemData->autoIncOffset;
			autoIncOffset = utcNow + 1; // 当前时间永远是区间中的最后一秒。
			break;
		default:
			autoIncPeriod = 0;
			autoIncOffset = 0;
			break;
		}
		if(autoIncPeriod == 0){
			LOG_EMPERY_CENTER_WARNING("Item auto increment period is zero? itemId = ", itemData->itemId);
			continue;
		}
		autoIncOffset %= autoIncPeriod;

		const auto oldCount = obj->get_count();
		const auto oldUpdatedTime = obj->get_updatedTime();

		const auto prevInterval = saturatedSub(oldUpdatedTime, autoIncOffset) / autoIncPeriod;
		const auto curInterval = saturatedSub(utcNow, autoIncOffset) / autoIncPeriod;
		LOG_EMPERY_CENTER_DEBUG("> Checking items: itemId = ", itemData->itemId, ", prevInterval = ", prevInterval, ", curInterval = ", curInterval);
		if(curInterval <= prevInterval){
			continue;
		}
		const auto intervalCount = curInterval - prevInterval;

		if(itemData->autoIncStep >= 0){
			if(oldCount < itemData->autoIncBound){
				const auto countToAdd = saturatedMul(static_cast<boost::uint64_t>(itemData->autoIncStep), intervalCount);
				const auto newCount = std::min(saturatedAdd(oldCount, countToAdd), itemData->autoIncBound);
				LOG_EMPERY_CENTER_DEBUG("> Adding items: itemId = ", itemData->itemId, ", oldCount = ", oldCount, ", newCount = ", newCount);
				transaction.emplace_back(ItemTransactionElement::OP_ADD, itemData->itemId, newCount - oldCount,
					ReasonIds::ID_AUTO_INCREMENT, itemData->autoIncType, itemData->autoIncOffset, 0);
			}
		} else {
			if(oldCount > itemData->autoIncBound){
				LOG_EMPERY_CENTER_DEBUG("> Removing items: itemId = ", itemData->itemId, ", initCount = ", itemData->initCount);
				const auto countToRemove = saturatedMul(static_cast<boost::uint64_t>(-itemData->autoIncStep), intervalCount);
				const auto newCount = std::max(saturatedSub(oldCount, countToRemove), itemData->autoIncBound);
				LOG_EMPERY_CENTER_DEBUG("> Removing items: itemId = ", itemData->itemId, ", oldCount = ", oldCount, ", newCount = ", newCount);
				transaction.emplace_back(ItemTransactionElement::OP_REMOVE, itemData->itemId, oldCount - newCount,
					ReasonIds::ID_AUTO_INCREMENT, itemData->autoIncType, itemData->autoIncOffset, 0);
			}
		}
		const auto newUpdatedTime = saturatedAdd(oldUpdatedTime, saturatedMul(autoIncPeriod, intervalCount));
		newTimestamps.emplace(obj, newUpdatedTime);
	}
	commitTransaction(transaction.data(), transaction.size(),
		[&]{ for(auto &p: newTimestamps){ p.first->set_updatedTime(p.second); } });

	if(forceSynchronizationWithClient){
		for(auto it = m_items.begin(); it != m_items.end(); ++it){
			synchronizeItemWithClient(getAccountUuid(), it->second);
		}
	}
}

ItemBox::ItemInfo ItemBox::get(ItemId itemId) const {
	PROFILE_ME;

	ItemInfo info = { };
	info.itemId = itemId;

	const auto it = m_items.find(itemId);
	if(it == m_items.end()){
		return info;
	}
	fillItemInfo(info, it->second);
	return info;
}
void ItemBox::getAll(std::vector<ItemBox::ItemInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_items.size());
	for(auto it = m_items.begin(); it != m_items.end(); ++it){
		ItemInfo info;
		fillItemInfo(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

ItemId ItemBox::commitTransactionNoThrow(const ItemTransactionElement *elements, std::size_t count,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	boost::shared_ptr<bool> withdrawn;
	boost::container::flat_map<boost::shared_ptr<MySql::Center_Item>, boost::uint64_t /* newCount */> tempResultMap;

	for(std::size_t i = 0; i < count; ++i){
		const auto operation  = elements[i].m_operation;
		const auto itemId = elements[i].m_itemId;
		const auto deltaCount = elements[i].m_deltaCount;

		if(deltaCount == 0){
			continue;
		}

		const auto reason = elements[i].m_reason;
		const auto param1 = elements[i].m_param1;
		const auto param2 = elements[i].m_param2;
		const auto param3 = elements[i].m_param3;

		switch(operation){
		case ItemTransactionElement::OP_NONE:
			break;

		case ItemTransactionElement::OP_ADD:
			{
				boost::shared_ptr<MySql::Center_Item> obj;
				{
					const auto it = m_items.find(itemId);
					if(it == m_items.end()){
						obj = boost::make_shared<MySql::Center_Item>(getAccountUuid().get(), itemId.get(), 0, 0);
						obj->asyncSave(true);
						m_items.emplace(itemId, obj);
					} else {
						obj = it->second;
					}
				}
				auto tempIt = tempResultMap.find(obj);
				if(tempIt == tempResultMap.end()){
					tempIt = tempResultMap.emplace_hint(tempIt, obj, obj->get_count());
				}
				const auto oldCount = tempIt->second;
				tempIt->second = checkedAdd(oldCount, deltaCount);
				const auto newCount = tempIt->second;

				const auto &accountUuid = getAccountUuid();
				LOG_EMPERY_CENTER_DEBUG("@ Item transaction: add: accountUuid = ", accountUuid,
					", itemId = ", itemId, ", oldCount = ", oldCount, ", deltaCount = ", deltaCount, ", newCount = ", newCount,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				if(!withdrawn){
					withdrawn = boost::make_shared<bool>(true);
				}
				Poseidon::asyncRaiseEvent(
					boost::make_shared<Events::ItemChanged>(accountUuid,
						itemId, oldCount, newCount, reason, param1, param2, param3),
					withdrawn);
			}
			break;

		case ItemTransactionElement::OP_REMOVE:
		case ItemTransactionElement::OP_REMOVE_SATURATED:
			{
				const auto it = m_items.find(itemId);
				if(it == m_items.end()){
					if(operation != ItemTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("Item not found: itemId = ", itemId);
						return itemId;
					}
					break;
				}
				const auto &obj = it->second;
				auto tempIt = tempResultMap.find(obj);
				if(tempIt == tempResultMap.end()){
					tempIt = tempResultMap.emplace_hint(tempIt, obj, obj->get_count());
				}
				const auto oldCount = tempIt->second;
				if(tempIt->second >= deltaCount){
					tempIt->second -= deltaCount;
				} else {
					if(operation != ItemTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("No enough items: itemId = ", itemId,
							", tempCount = ", tempIt->second, ", deltaCount = ", deltaCount);
						return itemId;
					}
					tempIt->second = 0;
				}
				const auto newCount = tempIt->second;

				const auto &accountUuid = getAccountUuid();
				LOG_EMPERY_CENTER_DEBUG("@ Item transaction: remove: accountUuid = ", accountUuid,
					", itemId = ", itemId, ", oldCount = ", oldCount, ", deltaCount = ", deltaCount, ", newCount = ", newCount,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				if(!withdrawn){
					withdrawn = boost::make_shared<bool>(true);
				}
				Poseidon::asyncRaiseEvent(
					boost::make_shared<Events::ItemChanged>(accountUuid,
						itemId, oldCount, newCount, reason, param1, param2, param3),
					withdrawn);
			}
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown item transaction operation: operation = ", (unsigned)operation);
			DEBUG_THROW(Exception, sslit("Unknown item transaction operation"));
		}
	}

	if(callback){
		callback();
	}

	for(auto it = tempResultMap.begin(); it != tempResultMap.end(); ++it){
		it->first->set_count(it->second);
	}
	if(withdrawn){
		*withdrawn = false;
	}

	for(auto it = tempResultMap.begin(); it != tempResultMap.end(); ++it){
		synchronizeItemWithClient(getAccountUuid(), it->first);
	}

	return ItemId();
}
void ItemBox::commitTransaction(const ItemTransactionElement *elements, std::size_t count,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	const auto insuffId = commitTransactionNoThrow(elements, count, callback);
	if(insuffId != ItemId()){
		LOG_EMPERY_CENTER_DEBUG("Insufficient items in item box: accountUuid = ", getAccountUuid(), ", insuffId = ", insuffId);
		DEBUG_THROW(Exception, sslit("Insufficient items in item box"));
	}
}

}
