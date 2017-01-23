#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include "../mmain.hpp"
#include "../singletons/account_map.hpp"
#include "../data/promotion.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/item_map.hpp"
#include "../item_ids.hpp"
#include "../events/synuser.hpp"

namespace EmperyPromotion {

SYNUSER_SERVLET("update", session, params){
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
	auto new_referrer                 = params.get("newReferrer");
	auto forced_update                = !params.get("forcedUpdate").empty();
	const auto &sign = params.at("sign");

	Poseidon::JsonObject root;

	const auto &md5_key = get_config<std::string>("synuser_md5key");
	const auto sign_md5 = Poseidon::md5_hash(login_name + new_login_name + phone_number + nick + password + deal_password +
		banned_until + gender + country + bank_account_name + bank_name + bank_account_number + bank_swift_code + remarks +
		level + new_referrer + md5_key);
	const auto sign_expected = Poseidon::Http::hex_encode(sign_md5.data(), sign_md5.size());
	if(sign != sign_expected){
		LOG_EMPERY_PROMOTION_WARNING("Unexpected sign from ", session->get_remote_info(),
			": sign_expected = ", sign_expected, ", sign = ", sign);
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "sign_invalid";
		return root;
	}

	Poseidon::async_raise_event(boost::make_shared<Events::SynUser>(session->get_remote_info().ip.get(), "update",
		Poseidon::Http::url_encoded_from_optional_map(params)));

	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "user_not_found";
		return root;
	}

	bool full_access = false;
	if(info.referrer_id){
		const auto &synuser_top = get_config<std::string>("synuser_top");
		auto temp_info = AccountMap::get(info.referrer_id);
		for(;;){
			if(Poseidon::has_none_flags_of(temp_info.flags, AccountMap::FL_VALID)){
				break;
			}
			if(::strcasecmp(temp_info.login_name.c_str(), synuser_top.c_str()) == 0){
				full_access = true;
				break;
			}
			temp_info = AccountMap::get(temp_info.referrer_id);
		}
	}

	if(!forced_update){
		const auto withdrawn_balance = ItemMap::get_count(info.account_id, ItemIds::ID_WITHDRAWN_BALANCE);
		if(withdrawn_balance > 0){
			root[sslit("state")] = "failed";
			root[sslit("errorcode")] = "withdrawal_pending";
			return root;
		}
	}

	if(!new_login_name.empty()){
		auto temp_info = AccountMap::get_by_login_name(new_login_name);
		if(Poseidon::has_any_flags_of(temp_info.flags, AccountMap::FL_VALID) && (temp_info.account_id != info.account_id)){
			root[sslit("state")] = "failed";
			root[sslit("errorcode")] = "duplicate_login_name";
			return root;
		}
	}

	if(!phone_number.empty()){
		std::vector<AccountMap::AccountInfo> temp_infos;
		AccountMap::get_by_phone_number(temp_infos, phone_number);
		if(!temp_infos.empty() && (temp_infos.front().account_id != info.account_id)){
			root[sslit("state")] = "failed";
			root[sslit("errorcode")] = "duplicate_phone_number";
			return root;
		}
	}
	if(!level.empty()){
		const auto num = boost::lexical_cast<std::uint64_t>(level);
		if(num == 0){
			level = "0";
		} else {
			const auto new_promotion_data = Data::Promotion::get_by_display_level(num);
			if(!new_promotion_data){
				root[sslit("state")] = "failed";
				root[sslit("errorcode")] = "unknown_level";
				return root;
			}
			level = boost::lexical_cast<std::string>(new_promotion_data->level);
		}
	}
	AccountId new_referrer_id;
	if(!new_referrer.empty()){
		auto referrer_info = AccountMap::get_by_login_name(new_referrer);
		if(Poseidon::has_none_flags_of(referrer_info.flags, AccountMap::FL_VALID)){
			root[sslit("state")] = "failed";
			root[sslit("errorcode")] = "no_such_referrer";
			return root;
		}
		new_referrer_id = referrer_info.account_id;
	}

	if(full_access && !new_login_name.empty()){
		AccountMap::set_login_name(info.account_id, std::move(new_login_name));
	}
	if(full_access && !phone_number.empty()){
		AccountMap::set_phone_number(info.account_id, std::move(phone_number));
	}
	if(!nick.empty()){
		AccountMap::set_nick(info.account_id, std::move(nick));
	}
//	if(!password.empty()){
//		AccountMap::set_password(info.account_id, std::move(password));
//	}
//	if(!deal_password.empty()){
//		AccountMap::set_deal_password(info.account_id, std::move(deal_password));
//	}
	if(full_access && !banned_until.empty()){
		AccountMap::set_banned_until(info.account_id, boost::lexical_cast<std::uint64_t>(banned_until));
	}

	if(full_access && !gender.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_GENDER, std::move(gender));
	}
	if(full_access && !country.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_COUNTRY, std::move(country));
	}
	if(full_access && !bank_account_name.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_BANK_ACCOUNT_NAME, std::move(bank_account_name));
	}
	if(full_access && !bank_name.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_BANK_NAME, std::move(bank_name));
	}
	if(full_access && !bank_account_number.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_BANK_ACCOUNT_NUMBER, std::move(bank_account_number));
	}
	if(full_access && !bank_swift_code.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_BANK_SWIFT_CODE, std::move(bank_swift_code));
	}
	if(full_access && !remarks.empty()){
		AccountMap::set_attribute(info.account_id, AccountMap::ATTR_REMARKS, std::move(remarks));
	}

	if(full_access && !level.empty()){
		AccountMap::set_level(info.account_id, boost::lexical_cast<std::uint64_t>(level));
	}
	if(full_access && new_referrer_id){
		AccountMap::set_referrer_id(info.account_id, new_referrer_id);
	}

	root[sslit("state")] = "success";
	return root;
}

}
