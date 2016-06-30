#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/st_account.hpp"
#include "../../../empery_center/src/msg/ts_account.hpp"
#include "../../../empery_center/src/msg/err_account.hpp"
#include "../account.hpp"
#include "../singletons/account_map.hpp"
#include "../mmain.hpp"
#include "../reason_ids.hpp"
#include <poseidon/json.hpp>
#include <poseidon/async_job.hpp>

namespace EmperyController {

CONTROLLER_SERVLET(Msg::ST_AccountAccumulatePromotionBonus, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::get_or_reload(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	const auto taxing_amount = req.taxing_amount;

	const auto get_config_array_of_doubles = [](const char *key){
		try {
			const auto json = get_config<Poseidon::JsonArray>(key);
			std::vector<double> vec;
			vec.reserve(json.size());
			for(const auto &elem : json){
				vec.emplace_back(elem.get<double>());
			}
			return vec;
		} catch(std::exception &e){
			LOG_EMPERY_CONTROLLER_ERROR("Error parsing array of doubles: key = ", key);
			throw;
		}
	};

	const auto bonus_ratio = get_config<double>("promotion_bonus_ratio");
	const auto level_bonus_array = get_config_array_of_doubles("promotion_level_bonus_array");
	const auto income_tax_array = get_config_array_of_doubles("promotion_income_tax_array");
	const auto level_bonus_extra_array = get_config_array_of_doubles("promotion_level_bonus_extra_array");

	const auto dividend_total = static_cast<std::uint64_t>(taxing_amount * bonus_ratio);
	const auto income_tax_ratio_total = std::accumulate(income_tax_array.begin(), income_tax_array.end(), 0.0);

	const auto send_tax_nothrow = [&](const boost::shared_ptr<Account> referrer, ReasonId reason_id, std::uint64_t amount) noexcept {
		try {
			Poseidon::enqueue_async_job(
				[=]{
			 		PROFILE_ME;

					auto using_controller = referrer->get_controller();
					if(!using_controller){
						referrer->set_controller(controller);
						using_controller = controller;
					}

					const auto referrer_uuid = referrer->get_account_uuid();
					const auto taxer_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(referrer_uuid.get()[0]));

					Msg::TS_AccountSendPromotionBonus sreq;
					sreq.account_uuid = referrer_uuid.str();
					sreq.taxer_uuid   = account_uuid.str();
					sreq.amount       = amount;
					sreq.reason       = reason_id.get();
					sreq.param1       = static_cast<std::int64_t>(taxer_uuid_head);
					sreq.param2       = referrer->get_promotion_level();
					sreq.param3       = 0;
					auto result = using_controller->send_and_wait(sreq);
					LOG_EMPERY_CONTROLLER_INFO("@> Promotion bonus sent: sreq = ", sreq,
						", code = ", result.first, ", msg = ", result.second);
				});
		} catch(std::exception &e){
			LOG_EMPERY_CONTROLLER_WARNING("std::exception thrown: what = ", e.what());
		}
	};

	std::deque<boost::shared_ptr<Account>> referrers;
	auto account_next = account;
	for(;;){
		const auto referrer_uuid = account_next->get_referrer_uuid();
		if(!referrer_uuid){
			break;
		}
		account_next = AccountMap::forced_reload(referrer_uuid);
		if(!account_next){
			LOG_EMPERY_CONTROLLER_WARNING("No such account: referrer_uuid = ", referrer_uuid);
			break;
		}
		LOG_EMPERY_CONTROLLER_DEBUG("> Referrer: referrer_uuid = ", referrer_uuid);
		referrers.emplace_back(account_next);
	}

	std::uint64_t dividend_accumulated = 0;
	for(auto it = referrers.begin(); it != referrers.end(); ++it){
		const auto &referrer = *it;

		const auto referrer_uuid       = referrer->get_account_uuid();
		const unsigned promotion_level = referrer->get_promotion_level();

		double my_ratio;
		if((promotion_level == 0) || level_bonus_array.empty()){
			my_ratio = 0;
		} else if(promotion_level < level_bonus_array.size()){
			my_ratio = level_bonus_array.at(promotion_level - 1);
		} else {
			my_ratio = level_bonus_array.back();
		}

		const auto my_max_dividend = my_ratio * dividend_total;
		LOG_EMPERY_CONTROLLER_DEBUG("> Current referrer: referrer_uuid = ", referrer_uuid,
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
		send_tax_nothrow(referrer, ReasonIds::ID_LEVEL_BONUS, my_dividend - tax_total);

		auto tit = income_tax_array.begin();
		for(auto bit = it + 1; (tit != income_tax_array.end()) && (bit != referrers.end()); ++bit){
			const auto &next_referrer = *bit;
			if(next_referrer->get_promotion_level() <= 1){
				continue;
			}
			const auto tax_amount = static_cast<std::uint64_t>(std::floor(my_dividend * (*tit) + 0.001));
			send_tax_nothrow(next_referrer, ReasonIds::ID_INCOME_TAX, tax_amount);
			++tit;
		}

		if(promotion_level == level_bonus_array.size()){
			auto eit = level_bonus_extra_array.begin();
			for(auto bit = it + 1; (eit != level_bonus_extra_array.end()) && (bit != referrers.end()); ++bit){
				const auto &next_referrer = *bit;
				if(next_referrer->get_promotion_level() != level_bonus_array.size()){
					continue;
				}
				const auto extra_amount = static_cast<std::uint64_t>(std::floor(dividend_total * (*eit) + 0.001));
				send_tax_nothrow(next_referrer, ReasonIds::ID_LEVEL_BONUS_EXTRA, extra_amount);
				++eit;
			}
		}
	}

	return Response();
}

CONTROLLER_SERVLET(Msg::ST_AccountAcquireToken, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::get_or_reload(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto locked_until = account->get_locked_until();
	if(now < locked_until){
		return Response(Msg::ERR_INVALIDATION_IN_PROGRESS);
	}

	LOG_EMPERY_CONTROLLER_DEBUG("Acquire token: controller = ", controller->get_remote_info(), ", account_uuid = ", account_uuid);
	const auto old_controller = account->get_controller();
	if(old_controller != controller){
		if(old_controller){
			try {
				Msg::TS_AccountInvalidate msg;
				msg.account_uuid = account_uuid.str();
				old_controller->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CONTROLLER_WARNING("std::exception thrown: what = ", e.what());
				// old_controller->shutdown(e.what());
				throw;
			}

			LOG_EMPERY_CONTROLLER_DEBUG("Waiting for account invalidation...");
			const auto invalidation_delay = get_config<std::uint64_t>("account_invalidation_delay", 10000);
			account->set_locked_until(saturated_add(now, invalidation_delay));

			return Response(Msg::ERR_INVALIDATION_IN_PROGRESS);
		}

		account->set_controller(controller);
	}

	return Response();
}

CONTROLLER_SERVLET(Msg::ST_AccountQueryToken, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	const auto old_controller = account->get_controller();
	if(old_controller != controller){
		return Response(Msg::ERR_CONTROLLER_TOKEN_NOT_ACQUIRED);
	}

	return Response();
}

CONTROLLER_SERVLET(Msg::ST_AccountInvalidate, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	std::vector<boost::shared_ptr<ControllerSession>> controllers;
	AccountMap::get_all_controllers(controllers);
	for(auto it = controllers.begin(); it != controllers.end(); ++it){
		const auto &other_controller = *it;
		if(other_controller == controller){
			continue;
		}
		try {
			Msg::TS_AccountInvalidate msg;
			msg.account_uuid = account_uuid.str();
			other_controller->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CONTROLLER_WARNING("std::exception thrown: what = ", e.what());
			// old_controller->shutdown(e.what());
			throw;
		}
	}

	return Response();
}

}
