#include "precompiled.hpp"
#include "account_utilities.hpp"
#include "mmain.hpp"
#include <poseidon/json.hpp>
#include <poseidon/string.hpp>
#include <poseidon/async_job.hpp>
#include "singletons/account_map.hpp"
#include "account.hpp"
#include "singletons/mail_box_map.hpp"
#include "mail_box.hpp"
#include "mail_data.hpp"
#include "item_ids.hpp"
#include "mail_type_ids.hpp"
#include "chat_message_slot_ids.hpp"
#include "data/global.hpp"

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

	const auto send_mail_nothrow = [](const boost::shared_ptr<Account> &referrer, MailTypeId mail_type_id,
		std::uint64_t count_to_add, const boost::shared_ptr<Account> &taxer) noexcept
	{
		if(count_to_add == 0){
			return;
		}
		const auto gold_count = count_to_add; // count_to_add / 2;
		const auto coin_count = 0;            // count_to_add / 2;

		try {
			const auto referrer_uuid = referrer->get_account_uuid();
			const auto mail_box = MailBoxMap::require(referrer_uuid);

			const auto mail_uuid = MailUuid(Poseidon::Uuid::random());
			const auto language_id = LanguageId(); // neutral

			const auto default_mail_expiry_duration = Data::Global::as_unsigned(Data::Global::SLOT_DEFAULT_MAIL_EXPIRY_DURATION);
			const auto expiry_duration = checked_mul(default_mail_expiry_duration, (std::uint64_t)60000);
			const auto utc_now = Poseidon::get_utc_time();

			const auto taxer_uuid = taxer->get_account_uuid();

			std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
			segments.reserve(2);
			segments.emplace_back(ChatMessageSlotIds::ID_TAXER,      taxer_uuid.str());
			segments.emplace_back(ChatMessageSlotIds::ID_TAX_AMOUNT, boost::lexical_cast<std::string>(count_to_add));

			boost::container::flat_map<ItemId, std::uint64_t> attachments;
			attachments.reserve(2);
			if(gold_count != 0){
				attachments.emplace(ItemIds::ID_GOLD,       gold_count);
			}
			if(coin_count != 0){
				attachments.emplace(ItemIds::ID_GOLD_COINS, coin_count);
			}

			const auto mail_data = boost::make_shared<MailData>(mail_uuid, language_id, utc_now,
				mail_type_id, AccountUuid(), std::string(), std::move(segments), std::move(attachments));
			MailBoxMap::insert_mail_data(mail_data);

			MailBox::MailInfo mail_info = { };
			mail_info.mail_uuid   = mail_uuid;
			mail_info.expiry_time = saturated_add(utc_now, expiry_duration);
			mail_info.system      = true;
			mail_box->insert(std::move(mail_info));
			LOG_EMPERY_CENTER_DEBUG("Promotion bonus mail sent: referrer_uuid = ", referrer_uuid,
				", mail_type_id = ", mail_type_id, ", taxer_uuid = ", taxer_uuid, ", count_to_add = ", count_to_add);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
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
		const auto tax_total = static_cast<std::uint64_t>(std::round(my_dividend * income_tax_ratio_total));
		Poseidon::enqueue_async_job(
			std::bind(send_mail_nothrow, referrer, MailTypeIds::ID_LEVEL_BONUS, my_dividend - tax_total, account),
			{ }, withdrawn);

		{
			auto tit = g_income_tax_array.begin();
			for(auto bit = it + 1; (tit != g_income_tax_array.end()) && (bit != referrers.end()); ++bit){
				const auto &next_referrer = *bit;
				if(next_referrer->get_promotion_level() <= 1){
					continue;
				}
				const auto tax_amount = static_cast<std::uint64_t>(std::round(my_dividend * (*tit)));
				Poseidon::enqueue_async_job(
					std::bind(send_mail_nothrow, next_referrer, MailTypeIds::ID_INCOME_TAX, tax_amount, referrer),
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
				const auto extra_amount = static_cast<std::uint64_t>(std::round(dividend_total * (*eit)));
				Poseidon::enqueue_async_job(
					std::bind(send_mail_nothrow, next_referrer, MailTypeIds::ID_LEVEL_BONUS_EXTRA, extra_amount, referrer),
					{ }, withdrawn);
				++eit;
			}
		}
	}

	*withdrawn = false;
}

}
