#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../item_ids.hpp"
#include "../item_transaction_element.hpp"
#include "../events/item.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("buyDiamonds", session, params){
	const auto &login_name = params.at("loginName");
	const auto &deal_password = params.at("dealPassword");
	const auto &serial = params.at("serial");
	const auto amount = boost::lexical_cast<std::uint64_t>(params.at("amount"));

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	if(AccountMap::get_password_hash(deal_password) != info.deal_password_hash){
		ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_DEAL_PASSWORD;
		ret[sslit("errorMessage")] = "Deal password is incorrect";
		return ret;
	}
	if((info.banned_until != 0) && (Poseidon::get_utc_time() < info.banned_until)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Account is banned";
		return ret;
	}

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(info.account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, amount,
		Events::ItemChanged::R_BUY_DIAMONDS, 0, 0, 0, serial);
	const auto insufficient_item_id = ItemMap::commit_transaction_nothrow(transaction.data(), transaction.size());
	if(insufficient_item_id){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ITEMS;
		ret[sslit("errorMessage")] = "No enough items";
		return ret;
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}

