#include "precompiled.hpp"
#include "item_box.hpp"
#include "transaction_element.hpp"
#include "msg/sc_item.hpp"
#include "mysql/item.hpp"
#include "singletons/player_session_map.hpp"
#include "events/item.hpp"
#include "player_session.hpp"
#include "data/item.hpp"
#include "reason_ids.hpp"

namespace EmperyCenter {

namespace {
	void fill_item_info(ItemBox::ItemInfo &info, const boost::shared_ptr<MySql::Center_Item> &obj){
		PROFILE_ME;

		info.item_id = ItemId(obj->get_item_id());
		info.count  = obj->get_count();
	}

	void fill_item_message(Msg::SC_ItemChanged &msg, const boost::shared_ptr<MySql::Center_Item> &obj){
		PROFILE_ME;

		msg.item_id = obj->get_item_id();
		msg.count   = obj->get_count();
	}
}

ItemBox::ItemBox(AccountUuid account_uuid)
	: m_account_uuid(account_uuid)
{
}
ItemBox::ItemBox(AccountUuid account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_Item>> &items)
	: ItemBox(account_uuid)
{
	for(auto it = items.begin(); it != items.end(); ++it){
		const auto &obj = *it;
		m_items.emplace(ItemId(obj->get_item_id()), obj);
	}
}
ItemBox::~ItemBox(){
}

void ItemBox::check_init_items(){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("Checking for init items: account_uuid = ", get_account_uuid());
	std::vector<ItemTransactionElement> transaction;
	std::vector<boost::shared_ptr<const Data::Item>> items_to_check;
	Data::Item::get_init(items_to_check);
	for(auto dit = items_to_check.begin(); dit != items_to_check.end(); ++dit){
		const auto &item_data = *dit;
		const auto it = m_items.find(item_data->item_id);
		if(it == m_items.end()){
			LOG_EMPERY_CENTER_DEBUG("> Adding items: item_id = ", item_data->item_id, ", init_count = ", item_data->init_count);
			transaction.emplace_back(ItemTransactionElement::OP_ADD, item_data->item_id, item_data->init_count,
				ReasonIds::ID_INIT_ITEMS, item_data->init_count, 0, 0);
		}
	}
	commit_transaction(transaction.data(), transaction.size());
}

void ItemBox::pump_status(bool force_synchronization_with_client){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	LOG_EMPERY_CENTER_DEBUG("Checking for auto increment items: account_uuid = ", get_account_uuid());
	std::vector<ItemTransactionElement> transaction;
	std::vector<boost::shared_ptr<const Data::Item>> items_to_check;
	Data::Item::get_auto_inc(items_to_check);
	boost::container::flat_map<boost::shared_ptr<MySql::Center_Item>, boost::uint64_t> new_timestamps;
	for(auto dit = items_to_check.begin(); dit != items_to_check.end(); ++dit){
		const auto &item_data = *dit;
		auto it = m_items.find(item_data->item_id);
		if(it == m_items.end()){
			auto obj = boost::make_shared<MySql::Center_Item>(get_account_uuid().get(), item_data->item_id.get(), 0, 0);
			obj->async_save(true);
			it = m_items.emplace(item_data->item_id, std::move(obj)).first;
		}
		const auto &obj = it->second;

		boost::uint64_t auto_inc_period, auto_inc_offset;
		switch(item_data->auto_inc_type){
		case Data::Item::AIT_HOURLY:
			auto_inc_period = 3600 * 1000;
			auto_inc_offset = item_data->auto_inc_offset;
			break;
		case Data::Item::AIT_DAILY:
			auto_inc_period = 24 * 3600 * 1000;
			auto_inc_offset = item_data->auto_inc_offset;
			break;
		case Data::Item::AIT_WEEKLY:
			auto_inc_period = 7 * 24 * 3600 * 1000;
			auto_inc_offset = item_data->auto_inc_offset + 3 * 24 * 3600 * 1000; // 注意 1970-01-01 是星期四。
			break;
		case Data::Item::AIT_PERIODIC:
			auto_inc_period = item_data->auto_inc_offset;
			auto_inc_offset = utc_now + 1; // 当前时间永远是区间中的最后一秒。
			break;
		default:
			auto_inc_period = 0;
			auto_inc_offset = 0;
			break;
		}
		if(auto_inc_period == 0){
			LOG_EMPERY_CENTER_WARNING("Item auto increment period is zero? item_id = ", item_data->item_id);
			continue;
		}
		auto_inc_offset %= auto_inc_period;

		const auto old_count = obj->get_count();
		const auto old_updated_time = obj->get_updated_time();

		const auto prev_interval = saturated_sub(saturated_add(old_updated_time, auto_inc_period), auto_inc_offset) / auto_inc_period;
		const auto cur_interval = saturated_sub(utc_now, auto_inc_offset) / auto_inc_period;
		LOG_EMPERY_CENTER_DEBUG("> Checking item: item_id = ", item_data->item_id,
			", prev_interval = ", prev_interval, ", cur_interval = ", cur_interval);
		if(cur_interval <= prev_interval){
			continue;
		}
		const auto interval_count = cur_interval - prev_interval;

		if(item_data->auto_inc_step >= 0){
			if(old_count < item_data->auto_inc_bound){
				const auto count_to_add = saturated_mul(static_cast<boost::uint64_t>(item_data->auto_inc_step), interval_count);
				const auto new_count = std::min(saturated_add(old_count, count_to_add), item_data->auto_inc_bound);
				LOG_EMPERY_CENTER_DEBUG("> Adding items: item_id = ", item_data->item_id, ", old_count = ", old_count, ", new_count = ", new_count);
				transaction.emplace_back(ItemTransactionElement::OP_ADD, item_data->item_id, new_count - old_count,
					ReasonIds::ID_AUTO_INCREMENT, item_data->auto_inc_type, item_data->auto_inc_offset, 0);
			}
		} else {
			if(old_count > item_data->auto_inc_bound){
				LOG_EMPERY_CENTER_DEBUG("> Removing items: item_id = ", item_data->item_id, ", init_count = ", item_data->init_count);
				const auto count_to_remove = saturated_mul(static_cast<boost::uint64_t>(-(item_data->auto_inc_step)), interval_count);
				const auto new_count = std::max(saturated_sub(old_count, count_to_remove), item_data->auto_inc_bound);
				LOG_EMPERY_CENTER_DEBUG("> Removing items: item_id = ", item_data->item_id, ", old_count = ", old_count, ", new_count = ", new_count);
				transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_data->item_id, old_count - new_count,
					ReasonIds::ID_AUTO_INCREMENT, item_data->auto_inc_type, item_data->auto_inc_offset, 0);
			}
		}
		const auto new_updated_time = saturated_add(old_updated_time, saturated_mul(auto_inc_period, interval_count));
		new_timestamps.emplace(obj, new_updated_time);
	}
	commit_transaction(transaction.data(), transaction.size(),
		[&]{
			for(auto &p: new_timestamps){
				p.first->set_updated_time(p.second);
			}
		});

	if(force_synchronization_with_client){
		const auto session = PlayerSessionMap::get(get_account_uuid());
		if(session){
			try {
				for(auto it = m_items.begin(); it != m_items.end(); ++it){
					Msg::SC_ItemChanged msg;
					fill_item_message(msg, it->second);
					session->send(msg);
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
}

ItemBox::ItemInfo ItemBox::get(ItemId item_id) const {
	PROFILE_ME;

	ItemInfo info = { };
	info.item_id = item_id;

	const auto it = m_items.find(item_id);
	if(it == m_items.end()){
		return info;
	}
	fill_item_info(info, it->second);
	return info;
}
void ItemBox::get_all(std::vector<ItemBox::ItemInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_items.size());
	for(auto it = m_items.begin(); it != m_items.end(); ++it){
		ItemInfo info;
		fill_item_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

ItemId ItemBox::commit_transaction_nothrow(const ItemTransactionElement *elements, std::size_t count,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	boost::shared_ptr<bool> withdrawn;
	boost::container::flat_map<boost::shared_ptr<MySql::Center_Item>, boost::uint64_t /* new_count */> temp_result_map;

	for(std::size_t i = 0; i < count; ++i){
		const auto operation  = elements[i].m_operation;
		const auto item_id = elements[i].m_some_id;
		const auto delta_count = elements[i].m_delta_count;

		if(delta_count == 0){
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
				auto it = m_items.find(item_id);
				if(it == m_items.end()){
					auto obj = boost::make_shared<MySql::Center_Item>(get_account_uuid().get(), item_id.get(), 0, 0);
					obj->async_save(true);
					it = m_items.emplace_hint(it, item_id, std::move(obj));
				}
				const auto &obj = it->second;
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_count());
				}
				const auto old_count = temp_it->second;
				temp_it->second = checked_add(old_count, delta_count);
				const auto new_count = temp_it->second;

				const auto &account_uuid = get_account_uuid();
				LOG_EMPERY_CENTER_DEBUG("@ Item transaction: add: account_uuid = ", account_uuid,
					", item_id = ", item_id, ", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				if(!withdrawn){
					withdrawn = boost::make_shared<bool>(true);
				}
				Poseidon::async_raise_event(
					boost::make_shared<Events::ItemChanged>(account_uuid,
						item_id, old_count, new_count, reason, param1, param2, param3),
					withdrawn);
			}
			break;

		case ItemTransactionElement::OP_REMOVE:
		case ItemTransactionElement::OP_REMOVE_SATURATED:
			{
				const auto it = m_items.find(item_id);
				if(it == m_items.end()){
					if(operation != ItemTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("Item not found: item_id = ", item_id);
						return item_id;
					}
					break;
				}
				const auto &obj = it->second;
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_count());
				}
				const auto old_count = temp_it->second;
				if(temp_it->second >= delta_count){
					temp_it->second -= delta_count;
				} else {
					if(operation != ItemTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("No enough items: item_id = ", item_id,
							", temp_count = ", temp_it->second, ", delta_count = ", delta_count);
						return item_id;
					}
					temp_it->second = 0;
				}
				const auto new_count = temp_it->second;

				const auto &account_uuid = get_account_uuid();
				LOG_EMPERY_CENTER_DEBUG("@ Item transaction: remove: account_uuid = ", account_uuid,
					", item_id = ", item_id, ", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				if(!withdrawn){
					withdrawn = boost::make_shared<bool>(true);
				}
				Poseidon::async_raise_event(
					boost::make_shared<Events::ItemChanged>(account_uuid,
						item_id, old_count, new_count, reason, param1, param2, param3),
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

	for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
		it->first->set_count(it->second);
	}
	if(withdrawn){
		*withdrawn = false;
	}

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
				Msg::SC_ItemChanged msg;
				fill_item_message(msg, it->first);
				session->send(msg);
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return ItemId();
}
void ItemBox::commit_transaction(const ItemTransactionElement *elements, std::size_t count,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	const auto insuff_id = commit_transaction_nothrow(elements, count, callback);
	if(insuff_id != ItemId()){
		LOG_EMPERY_CENTER_DEBUG("Insufficient items in item box: account_uuid = ", get_account_uuid(), ", insuff_id = ", insuff_id);
		DEBUG_THROW(Exception, sslit("Insufficient items in item box"));
	}
}

}
