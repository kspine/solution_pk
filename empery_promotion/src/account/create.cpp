#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/string.hpp>
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../singletons/global_status.hpp"
#include "../item_transaction_element.hpp"
#include "../msg/err_account.hpp"
#include "../data/promotion.hpp"
#include "../utilities.hpp"
#include "../events/account.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("create", session, params){
	const auto &payer_login_name = params.at("payerLoginName");
	const auto &deal_password = params.get("dealPassword");
	const auto &login_name = params.at("loginName");
	const auto &nick = params.at("nick");
	const auto level = boost::lexical_cast<boost::uint64_t>(params.at("level"));
	const auto &gender = params.at("gender");
	const auto &country = params.at("country");
	const auto &password = params.at("password");
	const auto &phone_number = params.at("phoneNumber");
	const auto &bank_account_name = params.get("bankAccountName");
	const auto &bank_name = params.get("bankName");
	const auto &bank_account_number = params.get("bankAccountNumber");
	const auto &referrer_login_name = params.get("referrerLoginName");
	const auto &bank_swift_code = params.get("bankSwiftCode");
	const auto &remarks = params.get("remarks");
	const auto &ip = params.get("ip");
	const auto &additional_cards_str = params.get("additionalCards");

	Poseidon::JsonObject ret;

	auto payer_info = AccountMap::get_by_login_name(payer_login_name);
	if(Poseidon::has_none_flags_of(payer_info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_PAYER;
		ret[sslit("errorMessage")] = "Payer is not found";
		return ret;
	}
	if(level != 0){
		if(AccountMap::get_password_hash(deal_password) != payer_info.deal_password_hash){
			ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_DEAL_PASSWORD;
			ret[sslit("errorMessage")] = "Deal password is incorrect";
			return ret;
		}
	}
	const auto utc_now = Poseidon::get_utc_time();
	if((payer_info.banned_until != 0) && (utc_now < payer_info.banned_until)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Payer is banned";
		return ret;
	}

	auto referrer_info = referrer_login_name.empty() ? payer_info : AccountMap::get_by_login_name(referrer_login_name);
	if(Poseidon::has_none_flags_of(referrer_info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_REFERRER;
		ret[sslit("errorMessage")] = "Referrer is not found";
		return ret;
	}
	if((referrer_info.banned_until != 0) && (utc_now < referrer_info.banned_until)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Referrer is banned";
		return ret;
	}

	boost::shared_ptr<const Data::Promotion> promotion_data;
	if(level != 0){
		promotion_data = Data::Promotion::get(level);
		if(!promotion_data){
			ret[sslit("errorCode")] = (int)Msg::ERR_UNKNOWN_ACCOUNT_LEVEL;
			ret[sslit("errorMessage")] = "Account level is not found";
			return ret;
		}
	} else {
		const auto first_balancing_time = GlobalStatus::get(GlobalStatus::SLOT_FIRST_BALANCING_TIME);
		if(utc_now < first_balancing_time){
			LOG_EMPERY_PROMOTION_DEBUG("Before first balancing...");
		} else {
			promotion_data = Data::Promotion::get_first();
		}
	}

	auto temp_info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_any_flags_of(temp_info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_DUPLICATE_LOGIN_NAME;
		ret[sslit("errorMessage")] = "Another account with the same login name already exists";
		return ret;
	}

	std::vector<AccountMap::AccountInfo> temp_infos;
	AccountMap::get_by_phone_number(temp_infos, phone_number);
	if(!temp_infos.empty()){
		ret[sslit("errorCode")] = (int)Msg::ERR_DUPLICATE_PHONE_NUMBER;
		ret[sslit("errorMessage")] = "Another account with the same phone number already exists";
		return ret;
	}

	const auto new_account_id = AccountMap::create(login_name, phone_number, nick, password, password, referrer_info.account_id, 0, ip);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_GENDER, gender);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_COUNTRY, country);
//	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_PHONE_NUMBER, phone_number);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_BANK_ACCOUNT_NAME, bank_account_name);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_BANK_NAME, bank_name);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_BANK_ACCOUNT_NUMBER, bank_account_number);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_BANK_SWIFT_CODE, bank_swift_code);
	AccountMap::set_attribute(new_account_id, AccountMap::ATTR_REMARKS, remarks);

	Poseidon::async_raise_event(
		boost::make_shared<Events::AccountCreated>(new_account_id, ip));

	const auto init_gold_coin_array = Poseidon::explode<boost::uint64_t>(',',
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

	boost::uint64_t additional_cards = 0;
	if(!additional_cards_str.empty()){
		additional_cards = boost::lexical_cast<boost::uint64_t>(additional_cards_str);
	}

	if(promotion_data){
		const auto result = try_upgrade_account(new_account_id, payer_info.account_id, true, promotion_data, remarks, additional_cards);
		ret[sslit("balanceToConsume")] = result.second;
		if(!result.first){
			ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE;
			ret[sslit("errorMessage")] = "No enough account balance";
			return ret;
		}
		accumulate_balance_bonus(new_account_id, payer_info.account_id, result.second, promotion_data->level);
	} else {
		ret[sslit("balanceToConsume")] = 0;
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
