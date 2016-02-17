#include "precompiled.hpp"
#include "account_utilities.hpp"
#include "mmain.hpp"
#include <poseidon/json.hpp>
#include <poseidon/string.hpp>
#include <poseidon/async_job.hpp>
#include "singletons/account_map.hpp"
#include "account.hpp"
#include "singletons/tax_box_map.hpp"
#include "tax_box.hpp"
#include "singletons/item_box_map.hpp"
#include "item_box.hpp"
#include "item_ids.hpp"
#include "transaction_element.hpp"
#include "reason_ids.hpp"

namespace EmperyCenter {

namespace {
	double              g_bonus_ratio;
	std::vector<double> g_level_bonus_array;
	std::vector<double> g_income_tax_array;
	std::vector<double> g_level_bonus_extra_array;

	MODULE_RAII_PRIORITY(/* handles */, 1000){
		const auto promotion_bonus_ratio = get_config<double>("promotion_bonus_ratio", 0.60);
		LOG_EMPERY_CENTER_DEBUG("> promotion_bonus_ratio = ", promotion_bonus_ratio);
		g_bonus_ratio = promotion_bonus_ratio;

		const auto promotion_level_bonus_array = get_config<Poseidon::JsonArray>("promotion_level_bonus_array");
		std::vector<double> temp_vec;
		temp_vec.reserve(promotion_level_bonus_array.size());
		for(auto it = promotion_level_bonus_array.begin(); it != promotion_level_bonus_array.end(); ++it){
			temp_vec.emplace_back(it->get<double>());
		}
		LOG_EMPERY_CENTER_DEBUG("> promotion_level_bonus_array = ", Poseidon::implode(',', temp_vec));
		g_level_bonus_array = std::move(temp_vec);

		const auto promotion_income_tax_array = get_config<Poseidon::JsonArray>("promotion_income_tax_array");
		temp_vec.clear();
		temp_vec.reserve(promotion_income_tax_array.size());
		for(auto it = promotion_income_tax_array.begin(); it != promotion_income_tax_array.end(); ++it){
			temp_vec.emplace_back(it->get<double>());
		}
		LOG_EMPERY_CENTER_DEBUG("> promotion_income_tax_array = ", Poseidon::implode(',', temp_vec));
		g_income_tax_array = std::move(temp_vec);

		const auto promotion_level_bonus_extra_array = get_config<Poseidon::JsonArray>("promotion_level_bonus_extra_array");
		temp_vec.clear();
		temp_vec.reserve(promotion_level_bonus_extra_array.size());
		for(auto it = promotion_level_bonus_extra_array.begin(); it != promotion_level_bonus_extra_array.end(); ++it){
			temp_vec.emplace_back(it->get<double>());
		}
		LOG_EMPERY_CENTER_DEBUG("> promotion_level_bonus_extra_array = ", Poseidon::implode(',', temp_vec));
		g_level_bonus_extra_array = std::move(temp_vec);
	}
}

void accumulate_promotion_bonus(AccountUuid account_uuid, std::uint64_t amount){
	PROFILE_ME;

	const auto send_tax_nothrow = [](const boost::shared_ptr<Account> &account, ReasonId reason,
		std::uint64_t amount_to_add, const boost::shared_ptr<Account> &taxer) noexcept
	{
		PROFILE_ME;

		if(amount_to_add == 0){
			return;
		}

		const auto account_uuid = account->get_account_uuid();
		const auto taxer_uuid = taxer->get_account_uuid();

		const auto item_box = ItemBoxMap::require(account_uuid);
		const auto tax_box = TaxBoxMap::require(account_uuid);

		LOG_EMPERY_CENTER_DEBUG("Promotion tax: account_uuid = ", account_uuid,
			", reason = ", reason, ", taxer_uuid = ", taxer_uuid, ", amount_to_add = ", amount_to_add);

		const auto taxer_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(taxer_uuid.get()[0]));
		const auto utc_now = Poseidon::get_utc_time();

		const auto old_gold_amount = item_box->get(ItemIds::ID_GOLD).count;
		const auto new_gold_amount = checked_add(old_gold_amount, amount_to_add);

		std::vector<ItemTransactionElement> transaction;
		transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemIds::ID_GOLD, amount_to_add,
			reason, taxer_uuid_head, account->get_promotion_level(), 0);
		item_box->commit_transaction(transaction, false,
			[&]{ tax_box->push(utc_now, taxer_uuid, reason, old_gold_amount, new_gold_amount); });
	};

	const auto withdrawn = boost::make_shared<bool>(true);

	const auto account = AccountMap::require(account_uuid);

	const auto dividend_total         = static_cast<std::uint64_t>(amount * g_bonus_ratio);
	const auto income_tax_ratio_total = std::accumulate(g_income_tax_array.begin(), g_income_tax_array.end(), 0.0);

	std::vector<boost::shared_ptr<Account>> referrers;
	auto referrer_uuid = account->get_referrer_uuid();
	while(referrer_uuid){
		auto referrer = AccountMap::require(referrer_uuid);
		referrer_uuid = referrer->get_referrer_uuid();
		referrers.emplace_back(std::move(referrer));
	}

	std::uint64_t dividend_accumulated = 0;

	for(auto it = referrers.begin(); it != referrers.end(); ++it){
		const auto &referrer = *it;

		const auto referrer_uuid       = referrer->get_account_uuid();
		const unsigned promotion_level = referrer->get_promotion_level();

		double my_ratio;
		if((promotion_level == 0) || g_level_bonus_array.empty()){
			my_ratio = 0;
		} else if(promotion_level < g_level_bonus_array.size()){
			my_ratio = g_level_bonus_array.at(promotion_level - 1);
		} else {
			my_ratio = g_level_bonus_array.back();
		}

		const auto my_max_dividend = my_ratio * dividend_total;
		LOG_EMPERY_CENTER_DEBUG("> Current referrer: referrer_uuid = ", referrer_uuid,
			", promotion_level = ", promotion_level, ", my_ratio = ", my_ratio,
			", dividend_accumulated = ", dividend_accumulated, ", my_max_dividend = ", my_max_dividend);

		// 级差制。
		if(dividend_accumulated >= my_max_dividend){
			continue;
		}
		const auto my_dividend = my_max_dividend - dividend_accumulated;
		dividend_accumulated = my_max_dividend;

		// 扣除个税即使没有上家（视为被系统回收）。
		const auto tax_total = static_cast<std::uint64_t>(std::floor(my_dividend * income_tax_ratio_total + 0.001));
		Poseidon::enqueue_async_job(
			std::bind(send_tax_nothrow, referrer, ReasonIds::ID_LEVEL_BONUS, my_dividend - tax_total, account),
			{ }, withdrawn);

		{
			auto tit = g_income_tax_array.begin();
			for(auto bit = it + 1; (tit != g_income_tax_array.end()) && (bit != referrers.end()); ++bit){
				const auto &next_referrer = *bit;
				if(next_referrer->get_promotion_level() <= 1){
					continue;
				}
				const auto tax_amount = static_cast<std::uint64_t>(std::floor(my_dividend * (*tit) + 0.001));
				Poseidon::enqueue_async_job(
					std::bind(send_tax_nothrow, next_referrer, ReasonIds::ID_INCOME_TAX, tax_amount, referrer),
					{ }, withdrawn);
				++tit;
			}
		}

		if(promotion_level == g_level_bonus_array.size()){
			auto eit = g_level_bonus_extra_array.begin();
			for(auto bit = it + 1; (eit != g_level_bonus_extra_array.end()) && (bit != referrers.end()); ++bit){
				const auto &next_referrer = *bit;
				if(next_referrer->get_promotion_level() != g_level_bonus_array.size()){
					continue;
				}
				const auto extra_amount = static_cast<std::uint64_t>(std::floor(dividend_total * (*eit) + 0.001));
				Poseidon::enqueue_async_job(
					std::bind(send_tax_nothrow, next_referrer, ReasonIds::ID_LEVEL_BONUS_EXTRA, extra_amount, referrer),
					{ }, withdrawn);
				++eit;
			}
		}
	}

	*withdrawn = false;
}

}
