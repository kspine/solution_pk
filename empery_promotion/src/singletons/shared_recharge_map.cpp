#include "../precompiled.hpp"
#include "shared_recharge_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../mysql/shared_recharge.hpp"
#include "../mmain.hpp"
#include "account_map.hpp"
#include "item_map.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"
#include "../data/promotion.hpp"
#include "../item_transaction_element.hpp"

namespace EmperyPromotion {

namespace {
	struct SharedRechargeElement {
		boost::shared_ptr<MySql::Promotion_SharedRecharge> obj;

		std::pair<AccountId, AccountId> account_pair;
		AccountId account_id;
		AccountId recharge_to_account_id;

		explicit SharedRechargeElement(boost::shared_ptr<MySql::Promotion_SharedRecharge> obj_)
			: obj(std::move(obj_))
			, account_pair(AccountId(obj->get_account_id()), AccountId(obj->get_recharge_to_account_id()))
			, account_id(obj->get_account_id()), recharge_to_account_id(obj->get_recharge_to_account_id())
		{
		}
	};

	MULTI_INDEX_MAP(SharedRechargeContainer, SharedRechargeElement,
		UNIQUE_MEMBER_INDEX(account_pair)
		MULTI_MEMBER_INDEX(account_id)
		MULTI_MEMBER_INDEX(recharge_to_account_id)
	)

	boost::weak_ptr<SharedRechargeContainer> g_shared_recharge_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto shared_recharge_map = boost::make_shared<SharedRechargeContainer>();
		LOG_EMPERY_PROMOTION_INFO("Loading shared recharge elements...");
		conn->execute_sql("SELECT * FROM `Promotion_SharedRecharge`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Promotion_SharedRecharge>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			shared_recharge_map->insert(SharedRechargeElement(std::move(obj)));
		}
		LOG_EMPERY_PROMOTION_INFO("Loaded ", shared_recharge_map->size(), " shared recharge element(s).");
		g_shared_recharge_map = shared_recharge_map;
		handles.push(shared_recharge_map);
	}

	void fill_shared_recharge_info(SharedRechargeMap::SharedRechargeInfo &info, const boost::shared_ptr<MySql::Promotion_SharedRecharge> &obj){
		PROFILE_ME;

		info.account_id             = AccountId(obj->get_account_id());
		info.recharge_to_account_id = AccountId(obj->get_recharge_to_account_id());
		info.updated_time           = obj->get_updated_time();
		info.state                  = static_cast<SharedRechargeMap::State>(obj->get_state());
		info.amount                 = obj->get_amount();
	}
}

SharedRechargeMap::SharedRechargeInfo SharedRechargeMap::get(AccountId account_id, AccountId recharge_to_account_id){
	PROFILE_ME;

	SharedRechargeInfo info = { };
	info.account_id = account_id;
	info.recharge_to_account_id = recharge_to_account_id;

	const auto shared_recharge_map = g_shared_recharge_map.lock();
	if(!shared_recharge_map){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge map is not loaded.");
		return info;
	}

	const auto it = shared_recharge_map->find<0>(std::make_pair(account_id, recharge_to_account_id));
	if(it == shared_recharge_map->end<0>()){
		return info;
	}
	fill_shared_recharge_info(info, it->obj);
	return info;
}
void SharedRechargeMap::get_all(std::vector<SharedRechargeMap::SharedRechargeInfo> &ret){
	PROFILE_ME;

	const auto shared_recharge_map = g_shared_recharge_map.lock();
	if(!shared_recharge_map){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge map is not loaded.");
		return;
	}

	ret.reserve(ret.size() + shared_recharge_map->size());
	for(auto it = shared_recharge_map->begin<0>(); it != shared_recharge_map->end<0>(); ++it){
		if(it->obj->get_state() == ST_DELETED){
			continue;
		}
		SharedRechargeInfo info;
		fill_shared_recharge_info(info, it->obj);
		ret.emplace_back(std::move(info));
	}
}
void SharedRechargeMap::get_by_account(std::vector<SharedRechargeMap::SharedRechargeInfo> &ret, AccountId account_id){
	PROFILE_ME;

	const auto shared_recharge_map = g_shared_recharge_map.lock();
	if(!shared_recharge_map){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge map is not loaded.");
		return;
	}

	const auto range = shared_recharge_map->equal_range<1>(account_id);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(it->obj->get_state() == ST_DELETED){
			continue;
		}
		SharedRechargeInfo info;
		fill_shared_recharge_info(info, it->obj);
		ret.emplace_back(std::move(info));
	}
}
void SharedRechargeMap::get_by_recharge_to_account(std::vector<SharedRechargeMap::SharedRechargeInfo> &ret, AccountId recharge_to_account_id){
	PROFILE_ME;

	const auto shared_recharge_map = g_shared_recharge_map.lock();
	if(!shared_recharge_map){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge map is not loaded.");
		return;
	}

	const auto range = shared_recharge_map->equal_range<2>(recharge_to_account_id);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(it->obj->get_state() == ST_DELETED){
			continue;
		}
		SharedRechargeInfo info;
		fill_shared_recharge_info(info, it->obj);
		ret.emplace_back(std::move(info));
	}
}

void SharedRechargeMap::request(std::vector<SharedRechargeMap::SharedRechargeInfo> &recharge_to_accounts,
	AccountId account_id, std::uint64_t amount)
{
	PROFILE_ME;

	const auto shared_recharge_map = g_shared_recharge_map.lock();
	if(!shared_recharge_map){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge map is not loaded.");
		DEBUG_THROW(Exception, sslit("Shared recharge map is not loaded."));
	}

	auto old_range = shared_recharge_map->equal_range<1>(account_id);
	for(auto it = old_range.first; it != old_range.second; ++it){
		if(it->obj->get_state() != ST_DELETED){
			LOG_EMPERY_PROMOTION_WARNING("Another shared recharge is in progress: account_id = ", account_id,
				", recharge_to_account_id = ", it->recharge_to_account_id);
			DEBUG_THROW(Exception, sslit("Another shared recharge is in progress"));
		}
	}

	std::vector<AccountMap::AccountInfo> candidate_accounts, temp_accounts;

	std::vector<boost::shared_ptr<const Data::Promotion>> promotion_data_all;
	Data::Promotion::get_all(promotion_data_all);
	for(auto it = promotion_data_all.begin(); it != promotion_data_all.end(); ++it){
		const auto &promotion_data = *it;
		if(!promotion_data->shared_recharge_enabled){
			continue;
		}
		LOG_EMPERY_PROMOTION_DEBUG("> Gathering accounts by level: level = ", promotion_data->level);
		temp_accounts.clear();
		AccountMap::get_by_level(temp_accounts, promotion_data->level);
		for(auto tit = temp_accounts.begin(); tit != temp_accounts.end(); ++tit){
			const auto recharge_to_account_id = tit->account_id;
			const bool can_share = AccountMap::cast_attribute<bool>(recharge_to_account_id, AccountMap::ATTR_SHARED_RECHARGE_ENABLED);
			if(!can_share){
				continue;
			}
			const auto balance_amount = ItemMap::get_count(recharge_to_account_id, ItemIds::ID_ACCOUNT_BALANCE);
			if(balance_amount < amount){
				continue;
			}
			candidate_accounts.emplace_back(std::move(*tit));
		}
	}

	const auto utc_now = Poseidon::get_utc_time();

	recharge_to_accounts.reserve(recharge_to_accounts.size() + candidate_accounts.size());
	try {
		for(auto tit = temp_accounts.begin(); tit != temp_accounts.end(); ++tit){
			const auto recharge_to_account_id = tit->account_id;
			LOG_EMPERY_PROMOTION_DEBUG(">> Requesting: recharge_to_account_id = ", recharge_to_account_id);

			auto rit = shared_recharge_map->find<0>(std::make_pair(account_id, recharge_to_account_id));
			if(rit == shared_recharge_map->end<0>()){
				auto obj = boost::make_shared<MySql::Promotion_SharedRecharge>(
					account_id.get(), recharge_to_account_id.get(), utc_now, ST_DELETED, 0);
				obj->async_save(true);
				rit = shared_recharge_map->insert<0>(SharedRechargeElement(std::move(obj))).first;
			}
			const auto &obj = rit->obj;

			obj->set_state(ST_REQUESTING);
			obj->set_amount(amount);

			SharedRechargeInfo info;
			fill_shared_recharge_info(info, obj);
			recharge_to_accounts.emplace_back(std::move(info));
		}
	} catch(...){
		old_range = shared_recharge_map->equal_range<1>(account_id);
		for(auto it = old_range.first; it != old_range.second; ++it){
			it->obj->set_state(ST_DELETED);
		}
		throw;
	}
}
void SharedRechargeMap::accept(AccountId account_id, AccountId recharge_to_account_id){
	PROFILE_ME;

	const auto shared_recharge_map = g_shared_recharge_map.lock();
	if(!shared_recharge_map){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge map is not loaded.");
		DEBUG_THROW(Exception, sslit("Shared recharge map is not loaded."));
	}

	const auto rit = shared_recharge_map->find<0>(std::make_pair(account_id, recharge_to_account_id));
	if(rit == shared_recharge_map->end<0>()){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge element not found: account_id = ", account_id,
			", recharge_to_account_id = ", recharge_to_account_id);
		DEBUG_THROW(Exception, sslit("Shared recharge element not found"));
	}
	if(rit->obj->get_state() != ST_REQUESTING){
		LOG_EMPERY_PROMOTION_WARNING("No requesting shared recharge found: account_id = ", account_id,
			", recharge_to_account_id = ", recharge_to_account_id, ", state = ", rit->obj->get_state());
		DEBUG_THROW(Exception, sslit("No requesting shared recharge found"));
	}
	const auto amount = rit->obj->get_amount();

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(recharge_to_account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, amount,
		Events::ItemChanged::R_SHARED_RECHARGE_LOCK, account_id.get(), recharge_to_account_id.get(), 0, std::string());
	ItemMap::commit_transaction(transaction.data(), transaction.size());

	auto old_range = shared_recharge_map->equal_range<1>(account_id);
	for(auto it = old_range.first; it != old_range.second; ++it){
		it->obj->set_state(ST_DELETED);
	}
	rit->obj->set_state(ST_FORMED);
}
void SharedRechargeMap::decline(AccountId account_id, AccountId recharge_to_account_id){
	PROFILE_ME;

	const auto shared_recharge_map = g_shared_recharge_map.lock();
	if(!shared_recharge_map){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge map is not loaded.");
		DEBUG_THROW(Exception, sslit("Shared recharge map is not loaded."));
	}

	const auto rit = shared_recharge_map->find<0>(std::make_pair(account_id, recharge_to_account_id));
	if(rit == shared_recharge_map->end<0>()){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge element not found: account_id = ", account_id,
			", recharge_to_account_id = ", recharge_to_account_id);
		DEBUG_THROW(Exception, sslit("Shared recharge element not found"));
	}
	if(rit->obj->get_state() != ST_REQUESTING){
		LOG_EMPERY_PROMOTION_WARNING("No requesting shared recharge found: account_id = ", account_id,
			", recharge_to_account_id = ", recharge_to_account_id, ", state = ", rit->obj->get_state());
		DEBUG_THROW(Exception, sslit("No requesting shared recharge found"));
	}
	const auto amount = rit->obj->get_amount();

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(recharge_to_account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, amount,
		Events::ItemChanged::R_SHARED_RECHARGE_UNLOCK, account_id.get(), recharge_to_account_id.get(), 0, std::string());
	ItemMap::commit_transaction(transaction.data(), transaction.size());

	rit->obj->set_state(ST_DELETED);
}
void SharedRechargeMap::commit(AccountId account_id, AccountId recharge_to_account_id){
	PROFILE_ME;

	const auto shared_recharge_map = g_shared_recharge_map.lock();
	if(!shared_recharge_map){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge map is not loaded.");
		DEBUG_THROW(Exception, sslit("Shared recharge map is not loaded."));
	}

	const auto rit = shared_recharge_map->find<0>(std::make_pair(account_id, recharge_to_account_id));
	if(rit == shared_recharge_map->end<0>()){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge element not found: account_id = ", account_id,
			", recharge_to_account_id = ", recharge_to_account_id);
		DEBUG_THROW(Exception, sslit("Shared recharge element not found"));
	}
	if(rit->obj->get_state() != ST_FORMED){
		LOG_EMPERY_PROMOTION_WARNING("No formed shared recharge found: account_id = ", account_id,
			", recharge_to_account_id = ", recharge_to_account_id, ", state = ", rit->obj->get_state());
		DEBUG_THROW(Exception, sslit("No formed shared recharge found"));
	}
	const auto amount = rit->obj->get_amount();

	const auto reward_ratio = get_config<double>("shared_recharge_reward_ratio", 0.003);
	const auto reward_amount = std::ceil(amount * reward_ratio - 0.001);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, amount - reward_amount,
		Events::ItemChanged::R_SHARED_RECHARGE_COMMIT, account_id.get(), recharge_to_account_id.get(), 0, std::string());
	transaction.emplace_back(recharge_to_account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, reward_amount,
		Events::ItemChanged::R_SHARED_RECHARGE_REWARD, account_id.get(), recharge_to_account_id.get(), 0, std::string());
	ItemMap::commit_transaction(transaction.data(), transaction.size());

	rit->obj->set_state(ST_DELETED);
}
void SharedRechargeMap::rollback(AccountId account_id, AccountId recharge_to_account_id){
	PROFILE_ME;

	const auto shared_recharge_map = g_shared_recharge_map.lock();
	if(!shared_recharge_map){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge map is not loaded.");
		DEBUG_THROW(Exception, sslit("Shared recharge map is not loaded."));
	}

	const auto rit = shared_recharge_map->find<0>(std::make_pair(account_id, recharge_to_account_id));
	if(rit == shared_recharge_map->end<0>()){
		LOG_EMPERY_PROMOTION_WARNING("Shared recharge element not found: account_id = ", account_id,
			", recharge_to_account_id = ", recharge_to_account_id);
		DEBUG_THROW(Exception, sslit("Shared recharge element not found"));
	}
	if(rit->obj->get_state() != ST_FORMED){
		LOG_EMPERY_PROMOTION_WARNING("No formed shared recharge found: account_id = ", account_id,
			", recharge_to_account_id = ", recharge_to_account_id, ", state = ", rit->obj->get_state());
		DEBUG_THROW(Exception, sslit("No formed shared recharge found"));
	}
	const auto amount = rit->obj->get_amount();

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(recharge_to_account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, amount,
		Events::ItemChanged::R_SHARED_RECHARGE_ROLLBACK, account_id.get(), recharge_to_account_id.get(), 0, std::string());
	ItemMap::commit_transaction(transaction.data(), transaction.size());

	rit->obj->set_state(ST_DELETED);
}

}
