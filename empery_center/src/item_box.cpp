#include "precompiled.hpp"
#include "item_box.hpp"
#include "msg/sc_item.hpp"
#include "mysql/item.hpp"
#include "singletons/player_session_map.hpp"
#include "events/item.hpp"
#include "player_session.hpp"
#include "events/item.hpp"
#include "checked_arithmetic.hpp"

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

	void checkItemAutoIncrement(const AccountUuid &accountUuid,
		const boost::shared_ptr<MySql::Center_Item> &obj, boost::uint64_t utcNow)
	{
		PROFILE_ME;

		const auto itemId = ItemId(obj->get_itemId());
		LOG_EMPERY_CENTER_DEBUG("Checking item auto increment: accountUuid = ", accountUuid, ", itemId = ", itemId);

		// TODO
		LOG_EMPERY_CENTER_FATAL("Checking item auto increment: accountUuid = ", accountUuid, ", itemId = ", itemId);
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
	for(auto it = m_items.begin(); it != m_items.end(); ++it){
		checkItemAutoIncrement(getAccountUuid(), it->second, utcNow);

		if(forceSynchronizationWithClient){
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

ItemId ItemBox::commitTransactionNoThrow(const ItemBox::ItemTransactionElement *elements, std::size_t count,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	boost::shared_ptr<bool> withdrawn;

	boost::container::flat_map<boost::shared_ptr<MySql::Center_Item>,
		boost::uint64_t /* newCount */> tempResultMap;

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
						obj = boost::make_shared<MySql::Center_Item>(
							getAccountUuid().get(), itemId.get(), 0, 0);
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
void ItemBox::commitTransaction(const ItemBox::ItemTransactionElement *elements, std::size_t count,
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
