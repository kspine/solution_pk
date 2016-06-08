#include "precompiled.hpp"
#include "auction_center.hpp"
#include "flag_guard.hpp"
#include "transaction_element.hpp"
#include "mysql/auction.hpp"
#include "events/auction.hpp"
#include "item_box.hpp"
#include "castle.hpp"
#include "reason_ids.hpp"
#include "data/castle.hpp"
#include "data/item.hpp"
#include "data/global.hpp"
#include "checked_arithmetic.hpp"
#include "singletons/player_session_map.hpp"
#include "singletons/item_box_map.hpp"
#include "player_session.hpp"
#include "msg/sc_auction.hpp"
#include "singletons/world_map.hpp"
#include <poseidon/async_job.hpp>

namespace EmperyCenter {

namespace {
	void fill_item_info(AuctionCenter::ItemInfo &info, const boost::shared_ptr<MySql::Center_AuctionTransfer> &obj){
		PROFILE_ME;

		info.item_id = ItemId(obj->get_item_id());
		info.count   = obj->get_item_count();
	}
	void fill_transfer_info(AuctionCenter::TransferInfo &info, const boost::shared_ptr<MySql::Center_AuctionTransfer> &obj){
		PROFILE_ME;

		info.map_object_uuid        = MapObjectUuid(obj->unlocked_get_map_object_uuid());
		info.item_id                = ItemId(obj->get_item_id());
		info.item_count             = obj->get_item_count();
		info.item_count_locked      = obj->get_item_count_locked();
		info.item_count_fee         = obj->get_item_count_fee();
		info.resource_id            = ResourceId(obj->get_resource_id());
		info.resource_amount_locked = obj->get_resource_amount_locked();
		info.resource_amount_fee    = obj->get_resource_amount_fee();
		info.created_time           = obj->get_created_time();
		info.due_time               = obj->get_due_time();
	}

	void fill_item_message(Msg::SC_AuctionItemChanged &msg, const boost::shared_ptr<MySql::Center_AuctionTransfer> &obj){
		PROFILE_ME;

		msg.item_id = obj->get_item_id();
		msg.count   = obj->get_item_count();
	}
	void fill_transfer_message(Msg::SC_AuctionTransferStatus &msg,
		MapObjectUuid map_object_uuid, const boost::container::flat_map<ItemId, boost::shared_ptr<MySql::Center_AuctionTransfer>> &transfers,
		std::uint64_t utc_now)
	{
		PROFILE_ME;

		msg.map_object_uuid = map_object_uuid.str();
		msg.items.reserve(transfers.size());
		for(auto it = transfers.begin(); it != transfers.end(); ++it){
			const auto &obj = it->second;
			auto &item = *msg.items.emplace(msg.items.end());
			item.item_id            = obj->get_item_id();
			item.item_count         = obj->get_item_count();
			item.created_time       = obj->get_created_time();
			item.due_time           = obj->get_due_time();
			item.remaining_duration = saturated_sub(obj->get_due_time(), utc_now);
		}
	}

	void check_transfer(const boost::shared_ptr<AuctionCenter> &auction_center,
		MapObjectUuid map_object_uuid, boost::container::flat_map<ItemId, boost::shared_ptr<MySql::Center_AuctionTransfer>> &transfers,
		std::uint64_t utc_now)
	{
		PROFILE_ME;

		const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
		if(!castle){
			LOG_EMPERY_CENTER_ERROR("No such castle: map_object_uuid = ", map_object_uuid);
			return;
		}

		bool forced_cancel = false;
		for(auto it = transfers.begin(); it != transfers.end(); ++it){
			const auto &obj = it->second;
			if(obj->get_created_time() == 0){
				continue;
			}
			const auto resource_id = ResourceId(obj->get_resource_id());
			if(!resource_id){
				continue;
			}
			if(obj->get_due_time() < utc_now){
				forced_cancel = false;
				goto _check_it_now;
			}
			const auto resource_data = Data::CastleResource::require(resource_id);
			const auto locked_resource_id = resource_data->locked_resource_id;
			const auto locked_info = castle->get_resource(locked_resource_id);
			const auto locked_amount = obj->get_resource_amount_locked();
			if(locked_info.amount < locked_amount){
				forced_cancel = true;
				goto _check_it_now;
			}
		}
		return;

	_check_it_now:
		Poseidon::enqueue_async_job(
			[=]{
				PROFILE_ME;
				LOG_EMPERY_CENTER_DEBUG("Committing transfer: account_uuid = ", auction_center->get_account_uuid(),
					", map_object_uuid = ", map_object_uuid);

				const auto item_box = ItemBoxMap::require(auction_center->get_account_uuid());

				if(!forced_cancel){
					const auto insuff_resource_id = auction_center->commit_transfer(castle);
					if(insuff_resource_id == ResourceId()){
						return;
					}
					LOG_EMPERY_CENTER_DEBUG("Failed to commit transfer. Cancel it.");
				}
				auction_center->cancel_transfer(castle, item_box, true);
			});
	}
}

AuctionCenter::AuctionCenter(AccountUuid account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_AuctionTransfer>> &transfers)
	: m_account_uuid(account_uuid)
{
	for(auto it = transfers.begin(); it != transfers.end(); ++it){
		const auto &obj = *it;
		const auto map_object_uuid = MapObjectUuid(obj->unlocked_get_map_object_uuid());
		const auto item_id = ItemId(obj->get_item_id());
		if(!map_object_uuid){
			m_items.emplace(item_id, obj);
		} else {
			m_transfers[map_object_uuid].emplace(item_id, obj);
		}
	}
}
AuctionCenter::~AuctionCenter(){
}

void AuctionCenter::pump_status(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	for(auto it = m_transfers.begin(); it != m_transfers.end(); ++it){
		check_transfer(virtual_shared_from_this<AuctionCenter>(), it->first, it->second, utc_now);
	}
}

AuctionCenter::ItemInfo AuctionCenter::get_item(ItemId item_id) const {
	PROFILE_ME;

	AuctionCenter::ItemInfo info = { };
	info.item_id = item_id;

	const auto it = m_items.find(item_id);
	if(it == m_items.end()){
		return info;
	}
	fill_item_info(info, it->second);
	return info;
}
void AuctionCenter::get_all_items(std::vector<AuctionCenter::ItemInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_items.size());
	for(auto it = m_items.begin(); it != m_items.end(); ++it){
		AuctionCenter::ItemInfo info;
		fill_item_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

ItemId AuctionCenter::commit_item_transaction_nothrow(const std::vector<AuctionTransactionElement> &transaction,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	std::vector<boost::shared_ptr<Poseidon::EventBaseWithoutId>> events;
	events.reserve(transaction.size());
	boost::container::flat_map<boost::shared_ptr<MySql::Center_AuctionTransfer>, std::uint64_t /* new_count */> temp_result_map;
	temp_result_map.reserve(transaction.size());

	const FlagGuard transaction_guard(m_locked_by_transaction);

	const auto account_uuid = get_account_uuid();

	for(auto tit = transaction.begin(); tit != transaction.end(); ++tit){
		const auto operation   = tit->m_operation;
		const auto item_id     = tit->m_some_id;
		const auto delta_count = tit->m_delta_count;

		if(delta_count == 0){
			continue;
		}

		const auto reason = tit->m_reason;
		const auto param1 = tit->m_param1;
		const auto param2 = tit->m_param2;
		const auto param3 = tit->m_param3;

		switch(operation){
		case AuctionTransactionElement::OP_NONE:
			break;

		case AuctionTransactionElement::OP_ADD:
			{
				auto it = m_items.find(item_id);
				if(it == m_items.end()){
					auto obj = boost::make_shared<MySql::Center_AuctionTransfer>(
						account_uuid.get(), Poseidon::Uuid(), item_id.get(), 0, 0, 0, 0, 0, 0, 0, 0);
					obj->async_save(true);
					it = m_items.emplace_hint(it, item_id, std::move(obj));
				}
				const auto &obj = it->second;
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_item_count());
				}
				const auto old_count = temp_it->second;
				temp_it->second = checked_add(old_count, delta_count);
				const auto new_count = temp_it->second;

				LOG_EMPERY_CENTER_DEBUG("% Auction transaction: add: account_uuid = ", account_uuid,
					", item_id = ", item_id, ", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::AuctionItemChanged>(
					account_uuid, item_id, old_count, new_count, reason, param1, param2, param3));
			}
			break;

		case AuctionTransactionElement::OP_REMOVE:
		case AuctionTransactionElement::OP_REMOVE_SATURATED:
			{
				const auto it = m_items.find(item_id);
				if(it == m_items.end()){
					if(operation != AuctionTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("Item not found: item_id = ", item_id);
						return item_id;
					}
					break;
				}
				const auto &obj = it->second;
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_item_count());
				}
				const auto old_count = temp_it->second;
				if(temp_it->second >= delta_count){
					temp_it->second -= delta_count;
				} else {
					if(operation != AuctionTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("No enough items: item_id = ", item_id,
							", temp_count = ", temp_it->second, ", delta_count = ", delta_count);
						return item_id;
					}
					temp_it->second = 0;
				}
				const auto new_count = temp_it->second;

				LOG_EMPERY_CENTER_DEBUG("% Auction transaction: remove: account_uuid = ", account_uuid,
					", item_id = ", item_id, ", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::AuctionItemChanged>(
					account_uuid, item_id, old_count, new_count, reason, param1, param2, param3));
			}
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown auction transaction operation: operation = ", (unsigned)operation);
			DEBUG_THROW(Exception, sslit("Unknown auction transaction operation"));
		}
	}

	const auto withdrawn = boost::make_shared<bool>(true);
	for(auto it = events.begin(); it != events.end(); ++it){
		Poseidon::async_raise_event(*it, withdrawn);
	}
	if(callback){
		callback();
	}
	for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
		it->first->set_item_count(it->second);
	}
	*withdrawn = false;

	return { };
}
void AuctionCenter::commit_item_transaction(const std::vector<AuctionTransactionElement> &transaction,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	const auto insuff_id = commit_item_transaction_nothrow(transaction, callback);
	if(insuff_id != ItemId()){
		LOG_EMPERY_CENTER_DEBUG("Insufficient items in auction center: account_uuid = ", get_account_uuid(), ", insuff_id = ", insuff_id);
		DEBUG_THROW(Exception, sslit("Insufficient items in auction center"));
	}
}

void AuctionCenter::get_transfer(std::vector<AuctionCenter::TransferInfo> &ret, MapObjectUuid map_object_uuid) const {
	PROFILE_ME;

	const auto tit = m_transfers.find(map_object_uuid);
	if(tit == m_transfers.end()){
		return;
	}

	ret.reserve(ret.size() + tit->second.size());
	for(auto it = tit->second.begin(); it != tit->second.end(); ++it){
		const auto &obj = it->second;
		if(obj->get_created_time() == 0){
			continue;
		}
		TransferInfo info;
		fill_transfer_info(info, obj);
		ret.emplace_back(std::move(info));
	}
}
void AuctionCenter::get_all_transfers(std::vector<AuctionCenter::TransferInfo> &ret) const {
	PROFILE_ME;

	for(auto tit = m_transfers.begin(); tit != m_transfers.end(); ++tit){
		ret.reserve(ret.size() + tit->second.size());
		for(auto it = tit->second.begin(); it != tit->second.end(); ++it){
			const auto &obj = it->second;
			if(obj->get_created_time() == 0){
				continue;
			}
			TransferInfo info;
			fill_transfer_info(info, obj);
			ret.emplace_back(std::move(info));
		}
	}
}
std::pair<ResourceId, ItemId> AuctionCenter::begin_transfer(const boost::shared_ptr<Castle> &castle, const boost::shared_ptr<ItemBox> &item_box,
	const boost::container::flat_map<ItemId, std::uint64_t> &items)
{
	PROFILE_ME;

	const auto account_uuid = get_account_uuid();

	const auto map_object_uuid = castle->get_map_object_uuid();
	auto &transfers = m_transfers[map_object_uuid];
	for(auto it = transfers.begin(); it != transfers.end(); ++it){
		const auto &obj = it->second;
		if(obj->get_created_time() == 0){
			continue;
		}
		LOG_EMPERY_CENTER_DEBUG("Another auction transfer is in progress: account_uuid = ", account_uuid,
			", map_object_uuid = ", map_object_uuid, ", item_id = ", obj->get_item_id());
		DEBUG_THROW(Exception, sslit("Another auction transfer is in progress"));
	}

	boost::container::flat_map<boost::shared_ptr<MySql::Center_AuctionTransfer>, TransferInfo> temp_result;
	temp_result.reserve(items.size());
	for(auto it = items.begin(); it != items.end(); ++it){
		const auto item_id = it->first;
		auto iit = transfers.find(item_id);
		if(iit == transfers.end()){
			auto obj = boost::make_shared<MySql::Center_AuctionTransfer>(
				account_uuid.get(), map_object_uuid.get(), item_id.get(), 0, 0, 0, 0, 0, 0, 0, 0);
			obj->async_save(true);
			iit = transfers.emplace(item_id, std::move(obj)).first;
		}
		const auto &obj = iit->second;
		temp_result.emplace(obj, TransferInfo());
	}

	const auto transfer_fee_ratio = Data::Global::as_double(Data::Global::SLOT_AUCTION_TRANSFER_FEE_RATIO);
	const auto transfer_duration  = Data::Global::as_double(Data::Global::SLOT_AUCTION_TRANSFER_DURATION);

	const auto resource_amount_per_box = Data::Global::as_unsigned(Data::Global::SLOT_AUCTION_TRANSFER_RESOURCE_AMOUNT_PER_BOX);
	const auto item_count_per_box = Data::Global::as_unsigned(Data::Global::SLOT_AUCTION_TRANSFER_ITEM_COUNT_PER_BOX);

	const auto utc_now = Poseidon::get_utc_time();
	const auto due_time = saturated_add(utc_now, static_cast<std::uint64_t>(transfer_duration * 60000));

	std::vector<ResourceTransactionElement> resource_transaction;
	resource_transaction.reserve(64);
	std::vector<ItemTransactionElement> item_transaction;
	item_transaction.reserve(16);

	for(auto it = items.begin(); it != items.end(); ++it){
		const auto item_id = it->first;
		const auto item_count = it->second;

		auto &info = temp_result.at(transfers.at(item_id));
		info.item_count = checked_add(info.item_count, item_count);

		const auto item_data = Data::Item::require(item_id);
		if(item_data->type.first == Data::Item::CAT_RESOURCE_BOX){
			const auto resource_id = ResourceId(item_data->type.second);
			const auto resource_data = Data::CastleResource::require(resource_id);
			const auto locked_resource_id = resource_data->locked_resource_id;

			const auto resource_amount_locked = checked_mul(item_count, resource_amount_per_box);
			const auto resource_amount_fee = static_cast<std::uint64_t>(std::ceil(resource_amount_locked * transfer_fee_ratio - 0.001));

			resource_transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, resource_id, resource_amount_locked,
				ReasonIds::ID_AUCTION_TRANSFER_LOCK, 0, 0, 0);
			resource_transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, resource_id, resource_amount_fee,
				ReasonIds::ID_AUCTION_TRANSFER_LOCK, 0, 0, 0);
			resource_transaction.emplace_back(ResourceTransactionElement::OP_ADD, locked_resource_id, resource_amount_locked,
				ReasonIds::ID_AUCTION_TRANSFER_LOCK, 0, 0, 0);

			info.resource_id            = resource_id;
			info.resource_amount_locked = checked_add(info.resource_amount_locked, resource_amount_locked);
			info.resource_amount_fee    = checked_add(info.resource_amount_fee,    resource_amount_fee);

			LOG_EMPERY_CENTER_DEBUG("%% Lock resource: map_object_uuid = ", map_object_uuid, ", item_id = ", item_id, ", item_count = ", item_count,
				", resource_id = ", resource_id, ", resource_amount_locked = ", resource_amount_locked,
				", resource_amount_fee = ", resource_amount_fee);
		} else {
			const auto item_count_locked = checked_mul(item_count, item_count_per_box);
			const auto item_count_fee = static_cast<std::uint64_t>(std::ceil(item_count_locked * transfer_fee_ratio - 0.001));

			item_transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, item_count_locked,
				ReasonIds::ID_AUCTION_TRANSFER_LOCK, 0, 0, 0);
			item_transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, item_count_fee,
				ReasonIds::ID_AUCTION_TRANSFER_LOCK, 0, 0, 0);

			info.item_count_locked      = checked_add(info.item_count_locked, item_count_locked);
			info.item_count_fee         = checked_add(info.item_count_fee,    item_count_fee);
			info.resource_id            = ResourceId();

			LOG_EMPERY_CENTER_DEBUG("%% Lock item: account_uuid = ", account_uuid, ", item_id = ", item_id, ", item_count = ", item_count,
				", item_count_locked = ", item_count_locked, ", item_count_fee = ", item_count_fee);
		}
	}

	try {
		const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(resource_transaction,
			[&]{
				const auto insuff_item_id = item_box->commit_transaction_nothrow(item_transaction, false,
					[&]{
						for(auto it = temp_result.begin(); it != temp_result.end(); ++it){
							const auto &obj = it->first;
							const auto &info = it->second;
							obj->set_item_count             (info.item_count);
							obj->set_item_count_locked      (info.item_count_locked);
							obj->set_item_count_fee         (info.item_count_fee);
							obj->set_resource_id            (info.resource_id.get());
							obj->set_resource_amount_locked (info.resource_amount_locked);
							obj->set_resource_amount_fee    (info.resource_amount_fee);
							obj->set_created_time           (utc_now);
							obj->set_due_time               (due_time);
						}
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

	return { };
}
ResourceId AuctionCenter::commit_transfer(const boost::shared_ptr<Castle> &castle){
	PROFILE_ME;

	const auto account_uuid = get_account_uuid();

	const auto map_object_uuid = castle->get_map_object_uuid();
	const auto tit = m_transfers.find(map_object_uuid);
	if(tit == m_transfers.end()){
		LOG_EMPERY_CENTER_DEBUG("No auction transfer is in progress: account_uuid = ", account_uuid, ", map_object_uuid = ", map_object_uuid);
		return { };
	}
	auto &transfers = tit->second;

	std::vector<boost::shared_ptr<MySql::Center_AuctionTransfer>> temp_result;
	temp_result.reserve(transfers.size());

	std::vector<AuctionTransactionElement> auction_transaction;
	auction_transaction.reserve(16);
	std::vector<ResourceTransactionElement> resource_transaction;
	resource_transaction.reserve(64);

	for(auto it = transfers.begin(); it != transfers.end(); ++it){
		const auto &obj = it->second;

		if(obj->get_created_time() == 0){
			continue;
		}

		const auto item_id = ItemId(obj->get_item_id());
		const auto item_count = obj->get_item_count();

		auction_transaction.emplace_back(AuctionTransactionElement::OP_ADD, item_id, item_count,
			ReasonIds::ID_AUCTION_TRANSFER_COMMIT, 0, 0, 0);

		const auto resource_id = ResourceId(obj->get_resource_id());
		if(resource_id){
			const auto resource_amount_locked = obj->get_resource_amount_locked();

			const auto resource_data = Data::CastleResource::require(resource_id);
			const auto locked_resource_id = resource_data->locked_resource_id;

			resource_transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, locked_resource_id, resource_amount_locked,
				ReasonIds::ID_AUCTION_TRANSFER_COMMIT, 0, 0, 0);

			LOG_EMPERY_CENTER_DEBUG("%% Commit resource: map_object_uuid = ", map_object_uuid,
				", item_id = ", item_id, ", item_count = ", item_count,
				", resource_id = ", resource_id, ", resource_amount_locked = ", resource_amount_locked);
		} else {
			const auto item_count_locked = obj->get_item_count_locked();

			LOG_EMPERY_CENTER_DEBUG("%% Commit item: account_uuid = ", account_uuid,
				", item_id = ", item_id, ", item_count = ", item_count,
				", item_count_locked = ", item_count_locked);
		}

		temp_result.emplace_back(obj);
	}

	try {
		commit_item_transaction(auction_transaction,
			[&]{
				const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(resource_transaction,
					[&]{
						for(auto it = temp_result.begin(); it != temp_result.end(); ++it){
							const auto &obj = *it;
							obj->set_item_count             (0);
							obj->set_item_count_locked      (0);
							obj->set_item_count_fee         (0);
							obj->set_resource_amount_locked (0);
							obj->set_resource_amount_fee    (0);
							obj->set_created_time           (0);
							obj->set_due_time               (0);
						}
					});
				if(insuff_resource_id){
					throw insuff_resource_id;
				}
			});
	} catch(ResourceId &insuff_resource_id){
		return insuff_resource_id;
	}

	return { };
}
void AuctionCenter::cancel_transfer(const boost::shared_ptr<Castle> &castle, const boost::shared_ptr<ItemBox> &item_box, bool refund_fee){
	PROFILE_ME;

	const auto account_uuid = get_account_uuid();

	const auto map_object_uuid = castle->get_map_object_uuid();
	const auto tit = m_transfers.find(map_object_uuid);
	if(tit == m_transfers.end()){
		LOG_EMPERY_CENTER_DEBUG("No auction transfer is in progress: account_uuid = ", account_uuid, ", map_object_uuid = ", map_object_uuid);
		return;
	}
	auto &transfers = tit->second;

	std::vector<boost::shared_ptr<MySql::Center_AuctionTransfer>> temp_result;
	temp_result.reserve(transfers.size());

	std::vector<ResourceTransactionElement> resource_transaction;
	resource_transaction.reserve(64);
	std::vector<ItemTransactionElement> item_transaction;
	item_transaction.reserve(16);

	for(auto it = transfers.begin(); it != transfers.end(); ++it){
		const auto &obj = it->second;

		if(obj->get_created_time() == 0){
			continue;
		}

		const auto resource_id = ResourceId(obj->get_resource_id());
		if(resource_id){
			const auto current_amount = castle->get_resource(resource_id).amount;

			const auto resource_amount_locked = std::min(obj->get_resource_amount_locked(), current_amount);
			const auto resource_amount_fee = obj->get_resource_amount_fee();

			const auto resource_data = Data::CastleResource::require(resource_id);
			const auto locked_resource_id = resource_data->locked_resource_id;

			resource_transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, resource_amount_locked,
				ReasonIds::ID_AUCTION_TRANSFER_UNLOCK, 0, 0, 0);
			if(refund_fee){
				resource_transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, resource_amount_fee,
					ReasonIds::ID_AUCTION_TRANSFER_UNLOCK, 0, 0, 0);
			}
			resource_transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, locked_resource_id, resource_amount_locked,
				ReasonIds::ID_AUCTION_TRANSFER_UNLOCK, 0, 0, 0);

			LOG_EMPERY_CENTER_DEBUG("%% Unlock resource: map_object_uuid = ", map_object_uuid,
				", resource_id = ", resource_id, ", resource_amount_locked = ", resource_amount_locked,
				", resource_amount_fee = ", resource_amount_fee);
		} else {
			const auto item_id = ItemId(obj->get_item_id());
			const auto item_count_locked = obj->get_item_count_locked();
			const auto item_count_fee = obj->get_item_count_fee();

			item_transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, item_count_locked,
				ReasonIds::ID_AUCTION_TRANSFER_UNLOCK, 0, 0, 0);
			if(refund_fee){
				item_transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, item_count_fee,
					ReasonIds::ID_AUCTION_TRANSFER_UNLOCK, 0, 0, 0);
			}

			LOG_EMPERY_CENTER_DEBUG("%% Unlock item: account_uuid = ", account_uuid,
				", item_id = ", item_id, ", item_count_locked = ", item_count_locked, ", item_count_fee = ", item_count_fee);
		}

		temp_result.emplace_back(obj);
	}

	castle->commit_resource_transaction(resource_transaction,
		[&]{
			item_box->commit_transaction(item_transaction, false,
				[&]{
					for(auto it = temp_result.begin(); it != temp_result.end(); ++it){
						const auto &obj = *it;
						obj->set_item_count             (0);
						obj->set_item_count_locked      (0);
						obj->set_item_count_fee         (0);
						obj->set_resource_amount_locked (0);
						obj->set_resource_amount_fee    (0);
						obj->set_created_time           (0);
					}
				});
		});
}

void AuctionCenter::pump_transfer_status(MapObjectUuid map_object_uuid){
	PROFILE_ME;

	const auto it = m_transfers.find(map_object_uuid);
	if(it == m_transfers.end()){
		LOG_EMPERY_CENTER_DEBUG("Auction transfer not found: account_uuid = ", get_account_uuid(), ", map_object_uuid = ", map_object_uuid);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	check_transfer(virtual_shared_from_this<AuctionCenter>(), it->first, it->second, utc_now);
}
void AuctionCenter::synchronize_transfer_with_player(MapObjectUuid map_object_uuid, const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto it = m_transfers.find(map_object_uuid);
	if(it == m_transfers.end()){
		LOG_EMPERY_CENTER_DEBUG("Auction transfer not found: account_uuid = ", get_account_uuid(), ", map_object_uuid = ", map_object_uuid);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	Msg::SC_AuctionTransferStatus msg;
	fill_transfer_message(msg, it->first, it->second, utc_now);
	session->send(msg);
}

void AuctionCenter::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	for(auto it = m_items.begin(); it != m_items.end(); ++it){
		Msg::SC_AuctionItemChanged msg;
		fill_item_message(msg, it->second);
		session->send(msg);
	}
	for(auto it = m_transfers.begin(); it != m_transfers.end(); ++it){
		Msg::SC_AuctionTransferStatus msg;
		fill_transfer_message(msg, it->first, it->second, utc_now);
		session->send(msg);
	}
}

}
