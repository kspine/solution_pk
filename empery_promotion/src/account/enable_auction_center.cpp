#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../item_ids.hpp"
#include "../item_transaction_element.hpp"
#include "../events/item.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("enableAuctionCenter", session, params){
	const auto &login_name = params.at("loginName");
	const auto &deal_password = params.at("dealPassword");

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

	const auto enabled = AccountMap::cast_attribute<bool>(info.account_id, AccountMap::ATTR_AUCTION_CENTER_ENABLED);
	if(enabled){
		ret[sslit("errorCode")] = (int)Msg::ERR_AUCTION_CENTER_ENABLED;
		ret[sslit("errorMessage")] = "Auction center is already enabled";
		return ret;
	}
/*
	const auto enable_price = get_config<std::uint64_t>("auction_center_enable_price", 50000);
*/
	std::string str("1");
	AccountMap::touch_attribute(info.account_id, AccountMap::ATTR_AUCTION_CENTER_ENABLED);
/*
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(info.account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, enable_price,
		Events::ItemChanged::R_ENABLE_AUCTION_CENTER, 0, 0, 0, std::string());
	const auto insufficient_item_id = ItemMap::commit_transaction_nothrow(transaction.data(), transaction.size());
	if(insufficient_item_id){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ITEMS;
		ret[sslit("errorMessage")] = "No enough items";
		return ret;
	}
*/	AccountMap::set_attribute(info.account_id, AccountMap::ATTR_AUCTION_CENTER_ENABLED, std::move(str));

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
