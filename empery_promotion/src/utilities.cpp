#include "precompiled.hpp"
#include "utilities.hpp"
#include "mmain.hpp"
#include "singletons/account_map.hpp"
#include "singletons/item_map.hpp"
#include "singletons/global_status.hpp"
#include "data/promotion.hpp"
#include "events/item.hpp"
#include "item_transaction_element.hpp"
#include "item_ids.hpp"
#include "mysql/outcome_balance_history.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/string.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <string>
#include <poseidon/singletons/job_dispatcher.hpp>

namespace EmperyPromotion {

namespace {
	struct StringCaseComparator {
		bool operator()(const std::string &lhs, const std::string &rhs) const {
			return ::strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
		}
	};
}

std::pair<bool, std::uint64_t> try_upgrade_account(AccountId account_id, AccountId payer_id, bool is_creating_account,
	const boost::shared_ptr<const Data::Promotion> &promotion_data, const std::string &remarks, std::uint64_t additional_cards)
{
	PROFILE_ME;
	LOG_EMPERY_PROMOTION_INFO("Upgrading account: account_id = ", account_id, ", level = ", promotion_data->level);

	const auto level = promotion_data->level;

	auto info = AccountMap::require(account_id);

	const double original_unit_price = GlobalStatus::get(GlobalStatus::SLOT_ACC_CARD_UNIT_PRICE);
	const std::uint64_t card_unit_price = std::ceil(original_unit_price * promotion_data->immediate_discount - 0.001);
	std::uint64_t balance_to_consume = 0;
	if(card_unit_price != 0){
		const auto min_cards_to_buy = static_cast<std::uint64_t>(
			std::ceil(static_cast<double>(promotion_data->immediate_price) / card_unit_price - 0.001));
		const auto cards_to_buy = checked_add(min_cards_to_buy, additional_cards);
		balance_to_consume = checked_mul(card_unit_price, cards_to_buy);
		LOG_EMPERY_PROMOTION_INFO("Acceleration cards: min_cards_to_buy = ", min_cards_to_buy, ", additional_cards = ", additional_cards,
			", cards_to_buy = ", cards_to_buy, ", balance_to_consume = ", balance_to_consume);

		auto reason = Events::ItemChanged::R_UPGRADE_ACCOUNT;
		if(is_creating_account){
			reason = Events::ItemChanged::R_CREATE_ACCOUNT;
		}

		std::vector<ItemTransactionElement> transaction;
		transaction.emplace_back(account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCELERATION_CARDS, cards_to_buy,
			reason, account_id.get(), payer_id.get(), level, remarks);
		transaction.emplace_back(payer_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, balance_to_consume,
			reason, account_id.get(), payer_id.get(), level, remarks);
		transaction.emplace_back(payer_id, ItemTransactionElement::OP_ADD, ItemIds::ID_BALANCE_BUY_CARDS_HISTORICAL, balance_to_consume,
			reason, account_id.get(), payer_id.get(), level, remarks);
		const auto insufficient_item_id = ItemMap::commit_transaction_nothrow(transaction.data(), transaction.size());
		if(insufficient_item_id){
			return std::make_pair(false, balance_to_consume);
		}
	}

	if(info.level < level){
		AccountMap::set_level(account_id, level);
	}
	// 放别处算。
	// check_auto_upgradeable(info.referrer_id);

	return std::make_pair(true, balance_to_consume);
}
void check_auto_upgradeable(AccountId init_account_id){
	PROFILE_ME;

	auto next_account_id = init_account_id;
	while(next_account_id){
		const auto account_id = next_account_id;
		const auto info = AccountMap::require(account_id);
		next_account_id = info.referrer_id;

		LOG_EMPERY_PROMOTION_DEBUG("> Next referrer: account_id = ", account_id, ", level = ", info.level);
		const auto old_promotion_data = Data::Promotion::get(info.level);
		if(!old_promotion_data){
			LOG_EMPERY_PROMOTION_DEBUG("> Referrer is at level zero.");
			continue;
		}

		boost::container::flat_map<std::uint64_t, std::uint64_t> subordinate_level_counts;
		subordinate_level_counts.reserve(32);
		{
			std::vector<AccountMap::AccountInfo> subordinates;
			AccountMap::get_by_referrer_id(subordinates, account_id);
			for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
				LOG_EMPERY_PROMOTION_DEBUG(">> Subordinate: account_id = ", it->account_id, ", max_level = ", it->max_level);
				++subordinate_level_counts[it->max_level];
			}
		}
		auto new_promotion_data = old_promotion_data;
		{
			auto cur_promotion_data = old_promotion_data;
			for(;;){
				auto next_promotion_data = Data::Promotion::get_next(cur_promotion_data->level);
				if(!next_promotion_data || (next_promotion_data == cur_promotion_data)){
					LOG_EMPERY_PROMOTION_DEBUG("No more promotion levels");
					break;
				}
				std::size_t count = 0;
				for(auto it = subordinate_level_counts.lower_bound(cur_promotion_data->level); it != subordinate_level_counts.end(); ++it){
					count += it->second;
				}
				LOG_EMPERY_PROMOTION_DEBUG("> Try auto upgrade: level = ", next_promotion_data->level,
					", count = ", count, ", auto_upgrade_count = ", next_promotion_data->auto_upgrade_count);
				if(count < next_promotion_data->auto_upgrade_count){
					LOG_EMPERY_PROMOTION_DEBUG("No enough subordinates");
				} else {
					new_promotion_data = next_promotion_data;
				}
				cur_promotion_data = std::move(next_promotion_data);
			}
		}
		if(new_promotion_data == old_promotion_data){
			LOG_EMPERY_PROMOTION_DEBUG("Not auto upgradeable: account_id = ", account_id);
		} else {
			LOG_EMPERY_PROMOTION_DEBUG("Auto upgrading: account_id = ", account_id, ", level = ", new_promotion_data->level);
			AccountMap::set_level(account_id, new_promotion_data->level);
		}
	}
}

namespace {
	double g_bonus_ratio;
	std::vector<double> g_income_tax_ratio_array, g_extra_tax_ratio_array;

	MODULE_RAII_PRIORITY(/* handles */, 1000){
		auto val = get_config<double>("balance_bonus_ratio", 0.70);
		LOG_EMPERY_PROMOTION_DEBUG("> Balance bonus ratio: ", val);
		g_bonus_ratio = val;

		auto str = get_config<std::string>("balance_income_tax_ratio_array", "0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01");
		auto ratio_array = Poseidon::explode<double>(',', str);
		LOG_EMPERY_PROMOTION_DEBUG("> Balance income tax ratio array: ", Poseidon::implode(',', ratio_array));
		g_income_tax_ratio_array = std::move(ratio_array);

		str = get_config<std::string>("balance_extra_tax_ratio_array", "0.02,0.02,0.01");
		ratio_array = Poseidon::explode<double>(',', str);
		LOG_EMPERY_PROMOTION_DEBUG("> Balance extra tax ratio array: ", Poseidon::implode(',', ratio_array));
		g_extra_tax_ratio_array = std::move(ratio_array);
	}

	void really_accumulate_balance_bonus(AccountId account_id, AccountId payer_id, std::uint64_t amount,
		AccountId virtual_referrer_id, std::uint64_t upgrade_to_level)
	{
		PROFILE_ME;
		LOG_EMPERY_PROMOTION_INFO("Balance bonus: account_id = ", account_id, ", payer_id = ", payer_id, ", amount = ", amount,
			", virtual_referrer_id = ", virtual_referrer_id, ", upgrade_to_level = ", upgrade_to_level);

		std::vector<ItemTransactionElement> transaction;
		transaction.reserve(64);

		const auto first_promotion_data = Data::Promotion::get_first();

		std::deque<std::pair<AccountId, boost::shared_ptr<const Data::Promotion>>> referrers;
		{
			auto current_id = virtual_referrer_id;
			while(current_id){
				auto referrer_info = AccountMap::require(current_id);
				LOG_EMPERY_PROMOTION_DEBUG("> Next referrer: current_id = ", current_id, ", level = ", referrer_info.level);
				referrers.emplace_back(current_id, Data::Promotion::get(referrer_info.level));
				current_id = referrer_info.referrer_id;
			}
		}

		const auto income_tax_ratio_total = std::accumulate(g_income_tax_ratio_array.begin(), g_income_tax_ratio_array.end(), 0.0);

		AccountMap::accumulate_self_performance(account_id, amount);

		std::uint64_t dividend_accumulated = 0;
		while(!referrers.empty()){
			const auto referrer_id = referrers.front().first;
			const auto referrer_promotion_data = std::move(referrers.front().second);
			referrers.pop_front();

			AccountMap::accumulate_performance(referrer_id, amount);

			if(!referrer_promotion_data){
				LOG_EMPERY_PROMOTION_DEBUG("> Referrer is at level zero: referrer_id = ", referrer_id);
				continue;
			}
			const auto my_max_dividend = amount * referrer_promotion_data->tax_ratio;
			LOG_EMPERY_PROMOTION_DEBUG("> Current referrer: referrer_id = ", referrer_id,
				", level = ", referrer_promotion_data->level, ", tax_ratio = ", referrer_promotion_data->tax_ratio,
				", dividend_accumulated = ", dividend_accumulated, ", my_max_dividend = ", my_max_dividend);

			// 级差制。
			if(dividend_accumulated >= my_max_dividend){
				continue;
			}
			const auto my_dividend = my_max_dividend - dividend_accumulated;
			dividend_accumulated = my_max_dividend;

			transaction.emplace_back(referrer_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, my_dividend,
				Events::ItemChanged::R_BALANCE_BONUS, account_id.get(), payer_id.get(), upgrade_to_level, std::string());

			// 扣除个税即使没有上家（视为被系统回收）。
			const auto tax_total = static_cast<std::uint64_t>(std::floor(my_dividend * income_tax_ratio_total + 0.001));
			transaction.emplace_back(referrer_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, tax_total,
				Events::ItemChanged::R_INCOME_TAX, my_dividend, referrer_id.get(), 0, std::string());

			{
				unsigned generation = 0;
				for(auto it = referrers.begin(); (generation < g_income_tax_ratio_array.size()) && (it != referrers.end()); ++it){
					if(!(it->second)){
						continue;
					}
					if(first_promotion_data && (it->second->level <= first_promotion_data->level)){
						continue;
					}
					const auto tax = static_cast<std::uint64_t>(std::floor(my_dividend * g_income_tax_ratio_array.at(generation) + 0.001));
					LOG_EMPERY_PROMOTION_DEBUG("> Referrer: referrer_id = ", it->first, ", level = ", it->second->level, ", tax = ", tax);
					transaction.emplace_back(it->first, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, tax,
						Events::ItemChanged::R_INCOME_TAX, my_dividend, referrer_id.get(), 0, std::string());
					++generation;
				}
			}

			if(referrer_promotion_data && referrer_promotion_data->tax_extra){
				unsigned generation = 0;
				for(auto it = referrers.begin(); (generation < g_extra_tax_ratio_array.size()) && (it != referrers.end()); ++it){
					if(!(it->second)){
						continue;
					}
					LOG_EMPERY_PROMOTION_DEBUG("> Referrer: referrer_id = ", it->first, ", level = ", it->second->level);
					if(!(it->second->tax_extra)){
						LOG_EMPERY_PROMOTION_DEBUG("> No extra tax available.");
						continue;
					}
					const auto extra = static_cast<std::uint64_t>(std::floor(amount * g_extra_tax_ratio_array.at(generation) + 0.001));
					LOG_EMPERY_PROMOTION_DEBUG("> Referrer: referrer_id = ", it->first, ", level = ", it->second->level, ", extra = ", extra);
					transaction.emplace_back(it->first, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, extra,
						Events::ItemChanged::R_BALANCE_BONUS_EXTRA, account_id.get(), payer_id.get(), upgrade_to_level, std::string());
					++generation;
				}
			}
		}

		ItemMap::commit_transaction(transaction.data(), transaction.size());
		LOG_EMPERY_PROMOTION_INFO("Done!");
	}
}

void commit_first_balance_bonus(){
	PROFILE_ME;
	LOG_EMPERY_PROMOTION_INFO("Committing first balancing...");

	std::map<std::string, boost::shared_ptr<const Data::Promotion>, StringCaseComparator> tout_accounts;
	{
		auto vec = get_config_v<std::string>("tout_account");
		for(auto it = vec.begin(); it != vec.end(); ++it){
			auto parts = Poseidon::explode<std::string>(',', *it, 2);
			auto login_name = std::move(parts.at(0));
			auto level = boost::lexical_cast<std::uint64_t>(parts.at(1));
			auto promotion_data = Data::Promotion::get(level);
			if(!promotion_data){
				LOG_EMPERY_PROMOTION_ERROR("Level not found: level = ", level);
				continue;
			}
			LOG_EMPERY_PROMOTION_DEBUG("> Tout account: login_name = ", login_name, ", level = ", promotion_data->level);
			tout_accounts[std::move(login_name)] = std::move(promotion_data);
		}
	}

	std::map<AccountId, std::deque<boost::shared_ptr<MySql::Promotion_OutcomeBalanceHistory>>> recharge_history;
	{
		std::vector<boost::shared_ptr<MySql::Promotion_OutcomeBalanceHistory>> objs;
		auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[&](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Promotion_OutcomeBalanceHistory>();
				obj->fetch(conn);
				objs.emplace_back(std::move(obj));
			}, "Promotion_OutcomeBalanceHistory", "SELECT * FROM `Promotion_OutcomeBalanceHistory` ORDER BY `timestamp` ASC, `auto_id` ASC");
		Poseidon::JobDispatcher::yield(promise, true);
		for(auto it = objs.begin(); it != objs.end(); ++it){
			auto &obj = *it;
			const auto account_id = AccountId(obj->get_account_id());
			recharge_history[account_id].emplace_back(std::move(obj));
		}
	}

	std::set<std::string, StringCaseComparator> fake_outcome;
	{
		auto fake_outcome_vec = get_config_v<std::string>("fake_outcome");
		for(auto it = fake_outcome_vec.begin(); it != fake_outcome_vec.end(); ++it){
			fake_outcome.insert(std::move(*it));
		}
	}

	const auto first_level_data = Data::Promotion::get_first();
	if(!first_level_data){
		LOG_EMPERY_PROMOTION_FATAL("No first level?");
		std::abort();
	}

	std::vector<AccountMap::AccountInfo> temp_infos;
	temp_infos.reserve(256);

	std::deque<AccountMap::AccountInfo> queue;
	AccountMap::get_by_referrer_id(temp_infos, AccountId(0));
	std::copy(std::make_move_iterator(temp_infos.begin()), std::make_move_iterator(temp_infos.end()), std::back_inserter(queue));
	while(!queue.empty()){
		const auto info = std::move(queue.front());
		queue.pop_front();
		LOG_EMPERY_PROMOTION_TRACE("> Processing: account_id = ", info.account_id, ", referrer_id = ", info.referrer_id);
		try {
			temp_infos.clear();
			AccountMap::get_by_referrer_id(temp_infos, info.account_id);
			std::copy(std::make_move_iterator(temp_infos.begin()), std::make_move_iterator(temp_infos.end()), std::back_inserter(queue));
		} catch(std::exception &e){
			LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
		}

		try {
			std::uint64_t new_level;
			const auto it = tout_accounts.find(info.login_name);
			if(it != tout_accounts.end()){
				new_level = it->second->level;
			} else {
				new_level = first_level_data->level;
			}
			AccountMap::set_level(info.account_id, new_level);
			check_auto_upgradeable(info.referrer_id);
		} catch(std::exception &e){
			LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
		}

		const auto queue_it = recharge_history.find(info.account_id);
		if(queue_it != recharge_history.end()){
			while(!queue_it->second.empty()){
				const auto obj = std::move(queue_it->second.front());
				queue_it->second.pop_front();

				std::uint64_t new_level;
				if(obj->get_reason() == Events::ItemChanged::R_UPGRADE_ACCOUNT){
					new_level = obj->get_param3();
				} else if(obj->get_reason() == Events::ItemChanged::R_CREATE_ACCOUNT){
					new_level = obj->get_param3();
				} else if(obj->get_reason() == Events::ItemChanged::R_BUY_MORE_CARDS){
					new_level = 0;
				} else {
					continue;
				}
				try {
					LOG_EMPERY_PROMOTION_DEBUG("> Accumulating: account_id = ", info.account_id,
						", outcome_balance = ", obj->get_outcome_balance(), ", reason = ", obj->get_reason());
					const auto old_level = info.level;
					if(fake_outcome.find(info.login_name) != fake_outcome.end()){
						LOG_EMPERY_PROMOTION_DEBUG("> Fake outcome user: login_name = ", info.login_name);
					} else {
						const auto amount = static_cast<std::uint64_t>(obj->get_outcome_balance() * g_bonus_ratio);
						really_accumulate_balance_bonus(info.account_id, info.account_id, amount,
							// 首次结算从自己开始，以后从推荐人开始。
							info.account_id, new_level);
					}
					const auto promotion_data = Data::Promotion::require(new_level);
					if(old_level < promotion_data->level){
						AccountMap::set_level(info.account_id, promotion_data->level);
					}
					check_auto_upgradeable(info.referrer_id);
				} catch(std::exception &e){
					LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
				}
			}
			recharge_history.erase(queue_it);
		}
	}
}
void accumulate_balance_bonus(AccountId account_id, AccountId payer_id, std::uint64_t amount, std::uint64_t upgrade_to_level){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();
	const auto first_balancing_time = GlobalStatus::get(GlobalStatus::SLOT_FIRST_BALANCING_TIME);
	if(utc_now < first_balancing_time){
		LOG_EMPERY_PROMOTION_DEBUG("Before first balancing...");
		return;
	}

	const auto info = AccountMap::require(account_id);
	really_accumulate_balance_bonus(account_id, payer_id, static_cast<std::uint64_t>(amount * g_bonus_ratio),
		info.referrer_id, upgrade_to_level);
}
void accumulate_balance_bonus_abs(AccountId account_id, std::uint64_t amount){
	PROFILE_ME;

	const auto info = AccountMap::require(account_id);
	really_accumulate_balance_bonus(account_id, account_id, amount,
		info.referrer_id, 0);
}

namespace {
	unsigned g_acceleration_card_alt_count;
	unsigned g_acceleration_card_alt_wrap;
	std::string g_acceleration_card_alt_account;

	MODULE_RAII_PRIORITY(/* handles */, 1000){
		auto val = get_config<unsigned>("acceleration_card_alt_count", 3);
		LOG_EMPERY_PROMOTION_DEBUG("> Acceleration sale alternative count: ", val);
		g_acceleration_card_alt_count = val;

		val = get_config<unsigned>("acceleration_card_alt_wrap", 5);
		LOG_EMPERY_PROMOTION_DEBUG("> Acceleration sale alternative wrap: ", val);
		g_acceleration_card_alt_wrap = val;

		auto str = get_config<std::string>("acceleration_card_alt_account", "google");
		LOG_EMPERY_PROMOTION_DEBUG("> Acceleration sale alternative account: ", str);
		g_acceleration_card_alt_account = std::move(str);
	}
}

std::uint64_t sell_acceleration_cards(AccountId buyer_id, std::uint64_t unit_price, std::uint64_t cards_to_sell){
	PROFILE_ME;

	std::deque<AccountMap::AccountInfo> queue;
	auto info = AccountMap::require(buyer_id);
	for(;;){
		const auto referrer_id = info.referrer_id;
		queue.emplace_back(std::move(info));
		if(!referrer_id){
			break;
		}
		info = AccountMap::require(referrer_id);
	}

	LOG_EMPERY_PROMOTION_DEBUG("Sell acceleration cards: buyer_id = ", buyer_id, ", unit_price = ", unit_price, ", cards_to_sell = ", cards_to_sell);

	std::uint64_t cards_sold = 0;

	for(std::uint64_t i = 0; i < cards_to_sell; ++i){
		std::vector<ItemTransactionElement> transaction;
		const auto state = GlobalStatus::fetch_add(GlobalStatus::SLOT_ACC_CARD_STATE, 1);
		const auto sale_alt = (g_acceleration_card_alt_wrap != 0) && ((state % g_acceleration_card_alt_wrap) < g_acceleration_card_alt_count);

		if(sale_alt){
			struct RewardElement {
				std::uint64_t min_level;
				std::uint64_t balance;
			};
			static constexpr RewardElement REWARDS[] = {
				{   50000, 2000 },
				{   50000, 2000 },
				{   50000, 2000 },
				{   50000, 2000 },
				{   50000, 2000 },
				{   50000, 2000 },
				{   50000, 2000 },
				{   50000, 2000 },
				{   50000, 2000 },
				{  100000, 1500 },
				{  200000, 1500 },
				{  300000, 1500 },
				{  500000, 1500 },
				{ 1000000, 1500 },
				{ 2000000, 1500 },
			};

			AccountMap::accumulate_self_performance(buyer_id, unit_price);
			for(auto qit = queue.begin() + 1; qit != queue.end(); ++qit){
				const auto account_id = qit->account_id;
				AccountMap::accumulate_performance(account_id, unit_price);
			}

			auto price_remaining = unit_price;
			auto reward_it = std::begin(REWARDS);
			for(auto qit = queue.begin() + 1; (qit != queue.end()) && (reward_it != std::end(REWARDS)); ++qit){
				const auto level = qit->level;
				if(level < reward_it->min_level){
					continue;
				}
				const auto account_id = qit->account_id;
				transaction.emplace_back(account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, reward_it->balance,
					Events::ItemChanged::R_SELL_CARDS_ALT, buyer_id.get(), 0, 0, std::string());
				price_remaining -= reward_it->balance;
				++reward_it;
			}
			auto alt_account = AccountMap::get_by_login_name(g_acceleration_card_alt_account);
			if(Poseidon::has_any_flags_of(alt_account.flags, AccountMap::FL_VALID)){
				transaction.emplace_back(alt_account.account_id, ItemTransactionElement::OP_REMOVE_SATURATED, ItemIds::ID_ACCELERATION_CARDS, 1,
					Events::ItemChanged::R_SELL_CARDS_ALT, buyer_id.get(), 0, 0, std::string());
				transaction.emplace_back(alt_account.account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, price_remaining,
					Events::ItemChanged::R_SELL_CARDS_ALT, buyer_id.get(), 0, 0, std::string());
			}
			cards_sold += 1;
		} else {
			for(auto qit = queue.begin(); qit != queue.end(); ++qit){
				const auto card_count = ItemMap::get_count(qit->account_id, ItemIds::ID_ACCELERATION_CARDS);
				LOG_EMPERY_PROMOTION_DEBUG("> Path upward: account_id = ", qit->account_id, ", card_count = ", card_count);
				if(card_count > 0){
					transaction.emplace_back(qit->account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCELERATION_CARDS, 1,
						Events::ItemChanged::R_SELL_CARDS, buyer_id.get(), 0, 0, std::string());
					transaction.emplace_back(qit->account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, unit_price,
						Events::ItemChanged::R_SELL_CARDS, buyer_id.get(), 0, 0, std::string());
					cards_sold += 1;
					goto _end;
				}
			}
			for(auto qit = queue.begin(); qit != queue.end(); ++qit){
				LOG_EMPERY_PROMOTION_DEBUG("Unpacking: account_id = ", qit->account_id);
				std::vector<AccountMap::AccountInfo> subordinates;
				AccountMap::get_by_referrer_id(subordinates, qit->account_id);
				if(qit != queue.begin()){
					const auto excluded_account_id = qit[-1].account_id;
					for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
						if(it->account_id == excluded_account_id){
							subordinates.erase(it);
							break;
						}
					}
				}
				while(!subordinates.empty()){
					std::sort(subordinates.begin(), subordinates.end(),
						[](const AccountMap::AccountInfo &lhs, const AccountMap::AccountInfo &rhs){
							return lhs.created_time < rhs.created_time;
						});
					for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
						const auto card_count = ItemMap::get_count(it->account_id, ItemIds::ID_ACCELERATION_CARDS);
						LOG_EMPERY_PROMOTION_DEBUG("> Path downward: account_id = ", it->account_id, ", card_count = ", card_count);
						if(card_count > 0){
							transaction.emplace_back(it->account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCELERATION_CARDS, 1,
								Events::ItemChanged::R_SELL_CARDS, buyer_id.get(), 0, 0, std::string());
							transaction.emplace_back(it->account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, unit_price,
								Events::ItemChanged::R_SELL_CARDS, buyer_id.get(), 0, 0, std::string());
							cards_sold += 1;
							goto _end;
						}
					}

					std::vector<AccountMap::AccountInfo> new_subordinates;
					for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
						LOG_EMPERY_PROMOTION_DEBUG("Unpacking: account_id = ", it->account_id);
						AccountMap::get_by_referrer_id(new_subordinates, it->account_id);
					}
					subordinates.swap(new_subordinates);
				}
			}
		_end:
			;
		}
		ItemMap::commit_transaction(transaction.data(), transaction.size());
	}
	return cards_sold;
}
std::uint64_t sell_acceleration_cards_internal(AccountId buyer_id, std::uint64_t unit_price, std::uint64_t cards_to_sell){
	PROFILE_ME;

	std::deque<AccountMap::AccountInfo> queue;
	auto info = AccountMap::require(buyer_id);
	for(;;){
		const auto referrer_id = info.referrer_id;
		queue.emplace_back(std::move(info));
		if(!referrer_id){
			break;
		}
		info = AccountMap::require(referrer_id);
	}

	LOG_EMPERY_PROMOTION_DEBUG("Sell acceleration cards: buyer_id = ", buyer_id, ", unit_price = ", unit_price, ", cards_to_sell = ", cards_to_sell);

	std::uint64_t cards_sold = 0;

	for(std::uint64_t i = 0; i < cards_to_sell; ++i){
		std::vector<ItemTransactionElement> transaction;
		for(auto qit = queue.begin(); qit != queue.end(); ++qit){
			if(qit->account_id == buyer_id){
				continue;
			}
			const auto card_count = ItemMap::get_count(qit->account_id, ItemIds::ID_ACCELERATION_CARDS);
			LOG_EMPERY_PROMOTION_DEBUG("> Path upward: account_id = ", qit->account_id, ", card_count = ", card_count);
			if(card_count > 0){
				transaction.emplace_back(qit->account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCELERATION_CARDS, 1,
					Events::ItemChanged::R_SELL_CARDS, buyer_id.get(), 0, 0, std::string());
				transaction.emplace_back(qit->account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, unit_price,
					Events::ItemChanged::R_SELL_CARDS, buyer_id.get(), 0, 0, std::string());
				cards_sold += 1;
				goto _end;
			}
		}
		for(auto qit = queue.begin(); qit != queue.end(); ++qit){
			LOG_EMPERY_PROMOTION_DEBUG("Unpacking: account_id = ", qit->account_id);
			std::vector<AccountMap::AccountInfo> subordinates;
			AccountMap::get_by_referrer_id(subordinates, qit->account_id);
			if(qit != queue.begin()){
				const auto excluded_account_id = qit[-1].account_id;
				for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
					if(it->account_id == excluded_account_id){
						subordinates.erase(it);
						break;
					}
				}
			}
			while(!subordinates.empty()){
				std::sort(subordinates.begin(), subordinates.end(),
					[](const AccountMap::AccountInfo &lhs, const AccountMap::AccountInfo &rhs){
						return lhs.created_time < rhs.created_time;
					});
				for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
					if(it->account_id == buyer_id){
						continue;
					}
					const auto card_count = ItemMap::get_count(it->account_id, ItemIds::ID_ACCELERATION_CARDS);
					LOG_EMPERY_PROMOTION_DEBUG("> Path downward: account_id = ", it->account_id, ", card_count = ", card_count);
					if(card_count > 0){
						transaction.emplace_back(it->account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCELERATION_CARDS, 1,
							Events::ItemChanged::R_SELL_CARDS, buyer_id.get(), 0, 0, std::string());
						transaction.emplace_back(it->account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, unit_price,
							Events::ItemChanged::R_SELL_CARDS, buyer_id.get(), 0, 0, std::string());
						cards_sold += 1;
						goto _end;
					}
				}

				std::vector<AccountMap::AccountInfo> new_subordinates;
				for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
					LOG_EMPERY_PROMOTION_DEBUG("Unpacking: account_id = ", it->account_id);
					AccountMap::get_by_referrer_id(new_subordinates, it->account_id);
				}
				subordinates.swap(new_subordinates);
			}
		}
	_end:
		transaction.emplace_back(buyer_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCELERATION_CARDS, 1,
			Events::ItemChanged::R_BUY_MORE_CARDS, buyer_id.get(), unit_price, 0, std::string());
		transaction.emplace_back(buyer_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, unit_price,
			Events::ItemChanged::R_BUY_MORE_CARDS, buyer_id.get(), unit_price, 0, std::string());
		ItemMap::commit_transaction(transaction.data(), transaction.size());
	}
	return cards_sold;
}
std::uint64_t sell_acceleration_cards_mojinpai(AccountId buyer_id, std::uint64_t unit_price, std::uint64_t cards_to_sell){
	PROFILE_ME;

	std::deque<AccountMap::AccountInfo> queue;
	auto info = AccountMap::require(buyer_id);
	for(;;){
		const auto referrer_id = info.referrer_id;
		queue.emplace_back(std::move(info));
		if(!referrer_id){
			break;
		}
		info = AccountMap::require(referrer_id);
	}

	LOG_EMPERY_PROMOTION_DEBUG("Sell acceleration cards: buyer_id = ", buyer_id, ", unit_price = ", unit_price, ", cards_to_sell = ", cards_to_sell);

	std::uint64_t cards_sold = 0;

	for(std::uint64_t i = 0; i < cards_to_sell; ++i){
		std::vector<ItemTransactionElement> transaction;
		for(auto qit = queue.begin(); qit != queue.end(); ++qit){
			const auto card_count = ItemMap::get_count(qit->account_id, ItemIds::ID_ACCELERATION_CARDS);
			LOG_EMPERY_PROMOTION_DEBUG("> Path upward: account_id = ", qit->account_id, ", card_count = ", card_count);
			if(card_count > 0){
				transaction.emplace_back(qit->account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCELERATION_CARDS, 1,
					Events::ItemChanged::R_SELL_CARDS_MOJINPAI, buyer_id.get(), 0, 0, std::string());
				transaction.emplace_back(qit->account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, unit_price,
					Events::ItemChanged::R_SELL_CARDS_MOJINPAI, buyer_id.get(), 0, 0, std::string());
				cards_sold += 1;
				goto _end;
			}
		}
		for(auto qit = queue.begin(); qit != queue.end(); ++qit){
			LOG_EMPERY_PROMOTION_DEBUG("Unpacking: account_id = ", qit->account_id);
			std::vector<AccountMap::AccountInfo> subordinates;
			AccountMap::get_by_referrer_id(subordinates, qit->account_id);
			if(qit != queue.begin()){
				const auto excluded_account_id = qit[-1].account_id;
				for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
					if(it->account_id == excluded_account_id){
						subordinates.erase(it);
						break;
					}
				}
			}
			while(!subordinates.empty()){
				std::sort(subordinates.begin(), subordinates.end(),
					[](const AccountMap::AccountInfo &lhs, const AccountMap::AccountInfo &rhs){
						return lhs.created_time < rhs.created_time;
					});
				for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
					const auto card_count = ItemMap::get_count(it->account_id, ItemIds::ID_ACCELERATION_CARDS);
					LOG_EMPERY_PROMOTION_DEBUG("> Path downward: account_id = ", it->account_id, ", card_count = ", card_count);
					if(card_count > 0){
						transaction.emplace_back(it->account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCELERATION_CARDS, 1,
							Events::ItemChanged::R_SELL_CARDS_MOJINPAI, buyer_id.get(), 0, 0, std::string());
						transaction.emplace_back(it->account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, unit_price,
							Events::ItemChanged::R_SELL_CARDS_MOJINPAI, buyer_id.get(), 0, 0, std::string());
						cards_sold += 1;
						goto _end;
					}
				}

				std::vector<AccountMap::AccountInfo> new_subordinates;
				for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
					LOG_EMPERY_PROMOTION_DEBUG("Unpacking: account_id = ", it->account_id);
					AccountMap::get_by_referrer_id(new_subordinates, it->account_id);
				}
				subordinates.swap(new_subordinates);
			}
		}
	_end:
		;
		ItemMap::commit_transaction(transaction.data(), transaction.size());
	}
	return cards_sold;
}

std::string generate_bill_serial(const std::string &prefix){
	PROFILE_ME;

	const auto auto_inc = GlobalStatus::fetch_add(GlobalStatus::SLOT_BILL_AUTO_INC, 1);

	std::string serial;
	serial.reserve(255);
	serial.append(prefix);
	const auto utc_now = Poseidon::get_utc_time();
	const auto dt = Poseidon::break_down_time(utc_now);
	char temp[256];
	unsigned len = (unsigned)std::sprintf(temp, "%04u%02u%02u%02u", dt.yr, dt.mon, dt.day, dt.hr);
	serial.append(temp, len);
	len = (unsigned)std::sprintf(temp, "%09u", (unsigned)auto_inc);
	serial.append(temp + len - 6, 6);
	return serial;
}

}
