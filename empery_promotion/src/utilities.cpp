#include "precompiled.hpp"
#include "utilities.hpp"
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

namespace EmperyPromotion {

namespace {
	struct StringCaseComparator {
		bool operator()(const std::string &lhs, const std::string &rhs) const {
			return ::strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
		}
	};

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
					LOG_EMPERY_PROMOTION_DEBUG("> > Subordinate: account_id = ", it->account_id, ", max_level = ", it->max_level);
					++subordinate_level_counts[it->max_level];
				}
			}
			auto new_promotion_data = old_promotion_data;
			for(;;){
				std::size_t count = 0;
				for(auto it = subordinate_level_counts.lower_bound(new_promotion_data->level); it != subordinate_level_counts.end(); ++it){
					count += it->second;
				}
				LOG_EMPERY_PROMOTION_DEBUG("> Try auto upgrade: level = ", new_promotion_data->level,
					", count = ", count, ", auto_upgrade_count = ", new_promotion_data->auto_upgrade_count);
				if(count < new_promotion_data->auto_upgrade_count){
					LOG_EMPERY_PROMOTION_DEBUG("No enough subordinates");
					break;
				}
				auto promotion_data = Data::Promotion::get_next(new_promotion_data->level);
				if(!promotion_data || (promotion_data == new_promotion_data)){
					LOG_EMPERY_PROMOTION_DEBUG("No more promotion levels");
					break;
				}
				new_promotion_data = std::move(promotion_data);
			}
			if(new_promotion_data == old_promotion_data){
				LOG_EMPERY_PROMOTION_DEBUG("Not auto upgradeable: account_id = ", account_id);
			} else {
				LOG_EMPERY_PROMOTION_DEBUG("Auto upgrading: account_id = ", account_id, ", level = ", new_promotion_data->level);
				AccountMap::set_level(account_id, new_promotion_data->level);
			}
		}
	}
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
	check_auto_upgradeable(info.referrer_id);

	return std::make_pair(true, balance_to_consume);
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
		for(auto current_id = virtual_referrer_id; current_id; current_id = AccountMap::require(current_id).referrer_id){
			auto referrer_info = AccountMap::require(current_id);
			LOG_EMPERY_PROMOTION_DEBUG("> Next referrer: current_id = ", current_id, ", level = ", referrer_info.level);
			referrers.emplace_back(current_id, Data::Promotion::get(referrer_info.level));
		}

		const auto dividend_total = static_cast<std::uint64_t>(amount * g_bonus_ratio);
		const auto income_tax_ratio_total = std::accumulate(g_income_tax_ratio_array.begin(), g_income_tax_ratio_array.end(), 0.0);

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
			const auto my_max_dividend = dividend_total * referrer_promotion_data->tax_ratio;
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
			const auto tax_total = static_cast<std::uint64_t>(std::round(my_dividend * income_tax_ratio_total));
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
					const auto tax = static_cast<std::uint64_t>(std::round(my_dividend * g_income_tax_ratio_array.at(generation)));
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
					const auto extra = static_cast<std::uint64_t>(std::round(dividend_total * g_extra_tax_ratio_array.at(generation)));
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
		std::vector<boost::shared_ptr<MySql::Promotion_OutcomeBalanceHistory>> temp_objs;
		MySql::Promotion_OutcomeBalanceHistory::batch_load(temp_objs,
			"SELECT * FROM `Promotion_OutcomeBalanceHistory` ORDER BY `timestamp` ASC, `auto_id` ASC");
		for(auto it = temp_objs.begin(); it != temp_objs.end(); ++it){
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
						really_accumulate_balance_bonus(info.account_id, info.account_id, obj->get_outcome_balance(),
							// 首次结算从自己开始，以后从推荐人开始。
							info.account_id, new_level);
					}
					if(old_level < new_level){
						AccountMap::set_level(info.account_id, new_level);
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
	really_accumulate_balance_bonus(account_id, payer_id, amount,
		info.referrer_id, upgrade_to_level);
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

	LOG_EMPERY_PROMOTION_DEBUG("Sell acceleration cards: buyer_id = ", buyer_id,
		", unit_price = ", unit_price, ", cards_to_sell = ", cards_to_sell);

	std::vector<ItemTransactionElement> transaction;
	std::uint64_t cards_sold = 0;

	const auto try_buy_and_update = [&](AccountId account_id, std::uint64_t card_count){
		const auto cards_to_sell_this_time = std::min(cards_to_sell - cards_sold, card_count);
		transaction.emplace_back(account_id, ItemTransactionElement::OP_REMOVE,
			ItemIds::ID_ACCELERATION_CARDS, cards_to_sell_this_time,
			Events::ItemChanged::R_SELL_CARDS, buyer_id.get(), 0, 0, std::string());
		transaction.emplace_back(account_id, ItemTransactionElement::OP_ADD,
			ItemIds::ID_ACCOUNT_BALANCE, checked_mul(cards_to_sell_this_time, unit_price),
			Events::ItemChanged::R_SELL_CARDS, buyer_id.get(), 0, 0, std::string());
		cards_sold += cards_to_sell_this_time;
		return cards_sold >= cards_to_sell;
	};

	for(auto it = queue.begin(); it != queue.end(); ++it){
		const auto card_count = ItemMap::get_count(it->account_id, ItemIds::ID_ACCELERATION_CARDS);
		LOG_EMPERY_PROMOTION_DEBUG("> Path upward: account_id = ", it->account_id, ", card_count = ", card_count);
		if(try_buy_and_update(it->account_id, card_count)){
			goto _end;
		}
	}
	while(!queue.empty()){
		std::vector<AccountMap::AccountInfo> subordinates;
		AccountMap::get_by_referrer_id(subordinates, queue.begin()->account_id);
		std::sort(subordinates.begin(), subordinates.end(),
			[](const AccountMap::AccountInfo &lhs, const AccountMap::AccountInfo &rhs){
				return lhs.created_time < rhs.created_time;
			});
		for(auto it = subordinates.begin(); it != subordinates.end(); ++it){
			const auto card_count = ItemMap::get_count(it->account_id, ItemIds::ID_ACCELERATION_CARDS);
			LOG_EMPERY_PROMOTION_DEBUG("> Path downward: account_id = ", it->account_id, ", card_count = ", card_count);
			if(try_buy_and_update(it->account_id, card_count)){
				goto _end;
			}
		}

		queue.pop_front();
		std::copy(std::make_move_iterator(subordinates.rbegin()), std::make_move_iterator(subordinates.rend()),
			std::front_inserter(queue));
	}

_end:
	ItemMap::commit_transaction(transaction.data(), transaction.size());
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
