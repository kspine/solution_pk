#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../item_transaction_element.hpp"
#include "../item_ids.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/string.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("deactivate", session, params){
	const auto &login_name = params.at("loginName");
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	if(Poseidon::has_any_flags_of(info.flags, AccountMap::FL_DEACTIVATED)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_DEACTIVATED;
		ret[sslit("errorMessage")] = "Account has already been deactivated";
		return ret;
	}

	const auto init_gold_coin_array = Poseidon::explode<std::uint64_t>(',',
	                               get_config<std::string>("init_gold_coins_array", "100,50,50"));

	std::vector<ItemTransactionElement> transaction;
	transaction.reserve(init_gold_coin_array.size());
	auto remove_gold_coins_from_whom = info.account_id;
	for(auto it = init_gold_coin_array.begin(); it != init_gold_coin_array.end(); ++it){
		transaction.emplace_back(remove_gold_coins_from_whom, ItemTransactionElement::OP_REMOVE_SATURATED, ItemIds::ID_GOLD_COINS, *it,
			Events::ItemChanged::R_DEACTIVATE_ACCOUNT, info.account_id.get(), 0, 0, remarks);

		const auto info = AccountMap::require(remove_gold_coins_from_whom);
		remove_gold_coins_from_whom = info.referrer_id;
		if(!remove_gold_coins_from_whom){
			break;
		}
	}
	ItemMap::commit_transaction(transaction.data(), transaction.size());

	AccountMap::set_phone_number(info.account_id, std::string());

	Poseidon::add_flags(info.flags, AccountMap::FL_DEACTIVATED);
	AccountMap::set_flags(info.account_id, info.flags);

	AccountMap::set_banned_until(info.account_id, (std::uint64_t)-1);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
