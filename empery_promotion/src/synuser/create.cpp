#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/string.hpp>
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include "../mmain.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/global_status.hpp"
#include "../msg/err_account.hpp"
#include "../data/promotion.hpp"
#include "../utilities.hpp"
#include "../events/account.hpp"

namespace EmperyPromotion {

SYNUSER_SERVLET("create", session, params){
	const auto &login_name = params.at("loginName");
	const auto &nick = params.at("nick");
	const auto &gender = params.at("gender");
	const auto &country = params.at("country");
	const auto &password = params.at("password");
	const auto &deal_password = params.at("dealPassword");
	const auto &phone_number = params.at("phoneNumber");
	const auto &bank_account_name = params.get("bankAccountName");
	const auto &bank_name = params.get("bankName");
	const auto &bank_account_number = params.get("bankAccountNumber");
	const auto &referrer_login_name = params.get("referrerLoginName");
	const auto &bank_swift_code = params.get("bankSwiftCode");
	const auto &remarks = params.get("remarks");
	const auto &ip = params.get("ip");
	const auto &sign = params.at("sign");

	Poseidon::JsonObject root;

	const auto &md5_key = get_config<std::string>("synuser_md5key");
	const auto sign_md5 = Poseidon::md5_hash(login_name + nick + gender + country + password + deal_password + phone_number +
		bank_account_name + bank_name + bank_account_number + referrer_login_name + bank_swift_code + remarks + ip + md5_key);
	const auto sign_expected = Poseidon::Http::hex_encode(sign_md5.data(), sign_md5.size());
	if(sign != sign_expected){
		LOG_EMPERY_PROMOTION_WARNING("Unexpected sign from ", session->get_remote_info(),
			": sign_expected = ", sign_expected, ", sign = ", sign);
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "sign_invalid";
		return root;
	}

	AccountMap::AccountInfo referrer_info = { };
	const auto &synuser_top = get_config<std::string>("synuser_top");
	if(referrer_login_name.empty()){
		referrer_info = AccountMap::get_by_login_name(synuser_top);
		if(Poseidon::has_none_flags_of(referrer_info.flags, AccountMap::FL_VALID)){
			root[sslit("state")] = "failed";
			root[sslit("errorcode")] = "no_such_referrer";
			return root;
		}
	} else {
		referrer_info = AccountMap::get_by_login_name(referrer_login_name);
		if(Poseidon::has_none_flags_of(referrer_info.flags, AccountMap::FL_VALID)){
			root[sslit("state")] = "failed";
			root[sslit("errorcode")] = "no_such_referrer";
			return root;
		}
		if(!referrer_info.referrer_id){
			root[sslit("state")] = "failed";
			root[sslit("errorcode")] = "access_denied";
			return root;
		}
		auto temp_info = AccountMap::get(referrer_info.referrer_id);
		for(;;){
			if(Poseidon::has_none_flags_of(temp_info.flags, AccountMap::FL_VALID)){
				root[sslit("state")] = "failed";
				root[sslit("errorcode")] = "access_denied";
				return root;
			}
			if(::strcasecmp(temp_info.login_name.c_str(), synuser_top.c_str()) == 0){
				break;
			}
			temp_info = AccountMap::get(temp_info.referrer_id);
		}
	}
	const auto utc_now = Poseidon::get_utc_time();
	if((referrer_info.banned_until != 0) && (utc_now < referrer_info.banned_until)){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "referrer_banned";
		return root;
	}

	boost::shared_ptr<const Data::Promotion> promotion_data;
	const auto first_balancing_time = GlobalStatus::get(GlobalStatus::SLOT_FIRST_BALANCING_TIME);
	if(utc_now < first_balancing_time){
		LOG_EMPERY_PROMOTION_DEBUG("Before first balancing...");
	} else {
		promotion_data = Data::Promotion::get_first();
	}

	auto temp_info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_any_flags_of(temp_info.flags, AccountMap::FL_VALID)){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "duplicate_login_name";
		return root;
	}

	std::vector<AccountMap::AccountInfo> temp_infos;
	AccountMap::get_by_phone_number(temp_infos, phone_number);
	if(!temp_infos.empty()){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "duplicate_phone_number";
		return root;
	}

	const auto new_account_id = AccountMap::create(login_name, phone_number, nick, password, deal_password, referrer_info.account_id, 0, ip);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_GENDER, gender);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_COUNTRY, country);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_BANK_ACCOUNT_NAME, bank_account_name);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_BANK_NAME, bank_name);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_BANK_ACCOUNT_NUMBER, bank_account_number);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_BANK_SWIFT_CODE, bank_swift_code);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_REMARKS, remarks);

	Poseidon::async_raise_event(
		boost::make_shared<Events::AccountCreated>(new_account_id, ip));
/*
	const auto init_gold_coin_array = Poseidon::explode<std::uint64_t>(',',
	                               get_config<std::string>("init_gold_coins_array"));
	std::vector<ItemTransactionElement> transaction;
	transaction.reserve(init_gold_coin_array.size());
	auto add_gold_coins_to_whom = new_account_id;
	for(auto it = init_gold_coin_array.begin(); it != init_gold_coin_array.end(); ++it){
		transaction.emplace_back(add_gold_coins_to_whom, ItemTransactionElement::OP_ADD, ItemIds::ID_GOLD_COINS, *it,
			Events::ItemChanged::R_CREATE_ACCOUNT, new_account_id.get(), payer_info.account_id.get(), level, remarks);

		const auto info = AccountMap::require(add_gold_coins_to_whom);
		add_gold_coins_to_whom = info.referrer_id;
		if(!add_gold_coins_to_whom){
			break;
		}
	}
	ItemMap::commit_transaction(transaction.data(), transaction.size());
*/
	if(promotion_data){
		const auto result = try_upgrade_account(new_account_id, referrer_info.account_id, true, promotion_data, remarks, 0);
//		root[sslit("balanceToConsume")] = result.second;
//		if(!result.first){
//			root[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE;
//			root[sslit("errorMessage")] = "No enough account balance";
//			return root;
//		}
		accumulate_balance_bonus(new_account_id, referrer_info.account_id, result.second, promotion_data->level);
	}
	check_auto_upgradeable(referrer_info.account_id);

	root[sslit("state")] = "success";
	return root;
}

}
