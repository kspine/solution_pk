#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../data/promotion.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/item_map.hpp"
#include "../item_ids.hpp"
#include "../events/admin.hpp"
#include <poseidon/http/utilities.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("setAccountAttributes", session, params){
	const auto &login_name            = params.at("loginName");
	auto new_login_name               = params.get("newLoginName");
	auto phone_number                 = params.get("phoneNumber");
	auto nick                         = params.get("nick");
	auto password                     = params.get("password");
	auto deal_password                = params.get("dealPassword");
	auto banned_until                 = params.get("bannedUntil");
	auto gender                       = params.get("gender");
	auto country                      = params.get("country");
	auto bank_account_name            = params.get("bankAccountName");
	auto bank_name                    = params.get("bankName");
	auto bank_account_number          = params.get("bankAccountNumber");
	auto bank_swift_code              = params.get("bankSwiftCode");
	auto remarks                      = params.get("remarks");
	auto level                        = params.get("level");
	auto max_visible_subord_depth     = params.get("maxVisibleSubordDepth");
	auto can_view_account_performance = params.get("canViewAccountPerformance");
	auto is_auction_center_enabled    = params.get("isAuctionCenterEnabled");
	auto is_shared_recharge_enabled   = params.get("isSharedRechargeEnabled");
	auto forced_update                = !params.get("forcedUpdate").empty();

	Poseidon::async_raise_event(boost::make_shared<Events::Admin>(session->get_remote_info().ip.get(), "setAccountAttributes",
		Poseidon::Http::url_encoded_from_optional_map(params)));

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	if(!forced_update){
		const auto withdrawn_balance = ItemMap::get_count(info.account_id, ItemIds::ID_WITHDRAWN_BALANCE);
		if(withdrawn_balance > 0){
			ret[sslit("errorCode")] = (int)Msg::ERR_WITHDRAWAL_PENDING;
			ret[sslit("errorMessage")] = "A withdrawal request is pending";
			return ret;
		}
	}

	if(!new_login_name.empty()){
		auto temp_info = AccountMap::get_by_login_name(new_login_name);
		if(Poseidon::has_any_flags_of(temp_info.flags, AccountMap::FL_VALID) && (temp_info.account_id != info.account_id)){
			ret[sslit("errorCode")] = (int)Msg::ERR_DUPLICATE_LOGIN_NAME;
			ret[sslit("errorMessage")] = "Another account with the same login name already exists";
			return ret;
		}
	}

	if(!phone_number.empty()){
		std::vector<AccountMap::AccountInfo> temp_infos;
		AccountMap::get_by_phone_number(temp_infos, phone_number);
		if(!temp_infos.empty() && (temp_infos.front().account_id != info.account_id)){
			ret[sslit("errorCode")] = (int)Msg::ERR_DUPLICATE_PHONE_NUMBER;
			ret[sslit("errorMessage")] = "Another account with the same phone number already exists";
			return ret;
		}
	}
	if(!level.empty()){
		const auto num = boost::lexical_cast<std::uint64_t>(level);
		if(num == 0){
			level = "0";
		} else {
			const auto new_promotion_data = Data::Promotion::get(num);
			if(!new_promotion_data){
				ret[sslit("errorCode")] = (int)Msg::ERR_UNKNOWN_ACCOUNT_LEVEL;
				ret[sslit("errorMessage")] = "Account level is not found";
				return ret;
			}
			level = boost::lexical_cast<std::string>(new_promotion_data->level);
		}
	}
	if(!max_visible_subord_depth.empty()){
		const auto depth = boost::lexical_cast<std::uint64_t>(max_visible_subord_depth);
		max_visible_subord_depth = boost::lexical_cast<std::string>(depth);
	}
	if(!can_view_account_performance.empty()){
		const auto visible = boost::lexical_cast<bool>(can_view_account_performance);
		can_view_account_performance = boost::lexical_cast<std::string>(visible);
	}
	if(!is_auction_center_enabled.empty()){
		const auto enabled = boost::lexical_cast<bool>(is_auction_center_enabled);
		is_auction_center_enabled = boost::lexical_cast<std::string>(enabled);
	}
	if(!is_shared_recharge_enabled.empty()){
		const auto enabled = boost::lexical_cast<bool>(is_shared_recharge_enabled);
		is_shared_recharge_enabled = boost::lexical_cast<std::string>(enabled);
	}

	if(!new_login_name.empty()){
		AccountMap::set_login_name(info.account_id, std::move(new_login_name));
	}
	if(!phone_number.empty()){
		AccountMap::set_phone_number(info.account_id, std::move(phone_number));
	}
	if(!nick.empty()){
		AccountMap::set_nick(info.account_id, std::move(nick));
	}
	if(!password.empty()){
		AccountMap::set_password(info.account_id, std::move(password));
	}
	if(!deal_password.empty()){
		AccountMap::set_deal_password(info.account_id, std::move(deal_password));
	}
	if(!banned_until.empty()){
		AccountMap::set_banned_until(info.account_id, boost::lexical_cast<std::uint64_t>(banned_until));
	}

	if(!gender.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_GENDER, std::move(gender));
	}
	if(!country.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_COUNTRY, std::move(country));
	}
	if(!bank_account_name.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_BANK_ACCOUNT_NAME, std::move(bank_account_name));
	}
	if(!bank_name.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_BANK_NAME, std::move(bank_name));
	}
	if(!bank_account_number.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_BANK_ACCOUNT_NUMBER, std::move(bank_account_number));
	}
	if(!bank_swift_code.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_BANK_SWIFT_CODE, std::move(bank_swift_code));
	}
	if(!remarks.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_REMARKS, std::move(remarks));
	}
	if(!level.empty()){
		AccountMap::set_level(info.account_id, boost::lexical_cast<std::uint64_t>(level));
	}
	if(!max_visible_subord_depth.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_MAX_VISIBLE_SUBORD_DEPTH, std::move(max_visible_subord_depth));
	}
	if(!can_view_account_performance.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_CAN_VIEW_ACCOUNT_PERFORMANCE, std::move(can_view_account_performance));
	}
	if(!is_auction_center_enabled.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_AUCTION_CENTER_ENABLED, std::move(is_auction_center_enabled));
	}
	if(!is_shared_recharge_enabled.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_SHARED_RECHARGE_ENABLED, std::move(is_shared_recharge_enabled));
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
