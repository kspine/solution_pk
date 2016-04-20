#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../item_ids.hpp"
#include "../item_transaction_element.hpp"
#include "../events/item.hpp"
#include "../data/promotion.hpp"
#include "../utilities.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("buyGiftBox", session, params){
	const auto &login_name = params.at("loginName");
	const auto &deal_password = params.at("dealPassword");
	const auto &serial = params.at("serial");
	const auto level = boost::lexical_cast<std::uint64_t>(params.at("level"));

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

	const auto promotion_data = Data::Promotion::get(level);
	if(!promotion_data){
		ret[sslit("errorCode")] = (int)Msg::ERR_UNKNOWN_ACCOUNT_LEVEL;
		ret[sslit("errorMessage")] = "Unknown account level";
		return ret;
	}
	if(promotion_data->immediate_price == (std::uint64_t)-1){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_LEVEL_NOT_FOR_SALE;
		ret[sslit("errorMessage")] = "Account level not for sale";
		return ret;
	}
	const auto price = promotion_data->immediate_price;

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(info.account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, price,
		Events::ItemChanged::R_BUY_GIFT_BOX, level, 0, 0, serial);
	const auto insufficient_item_id = ItemMap::commit_transaction_nothrow(transaction.data(), transaction.size());
	if(insufficient_item_id){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ITEMS;
		ret[sslit("errorMessage")] = "No enough items";
		return ret;
	}

	if(info.level < promotion_data->level){
		AccountMap::set_level(info.account_id, promotion_data->level);
	}
	check_auto_upgradeable(info.referrer_id);

	AccountMap::accumulate_self_performance(info.account_id, price);
	while(info.referrer_id){
		LOG_EMPERY_PROMOTION_DEBUG("> Accumulate performance: account_id = ", info.referrer_id, ", price = ", price);
		AccountMap::accumulate_performance(info.referrer_id, price);
		info = AccountMap::require(info.referrer_id);
	}

	ret[sslit("displayLevel")] = promotion_data->display_level;

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
