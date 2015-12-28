#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../data/promotion.hpp"
#include "../singletons/item_map.hpp"
#include "../singletons/global_status.hpp"
#include "../data/promotion.hpp"
#include "../events/item.hpp"
#include "../item_transaction_element.hpp"
#include "../item_ids.hpp"
#include "../utilities.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("buyMoreCards", session, params){
	const auto &login_name = params.at("loginName");
	const auto cards_to_buy = boost::lexical_cast<std::uint64_t>(params.at("cardsToBuy"));
	const auto &deal_password = params.at("dealPassword");
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;

	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_PAYER;
		ret[sslit("errorMessage")] = "Payer is not found";
		return ret;
	}
	if(AccountMap::get_password_hash(deal_password) != info.deal_password_hash){
		ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_DEAL_PASSWORD;
		ret[sslit("errorMessage")] = "Deal password is incorrect";
		return ret;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if((info.banned_until != 0) && (utc_now < info.banned_until)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Payer is banned";
		return ret;
	}

	double discount = 1.0;
	if(info.level != 0){
		const auto promotion_data = Data::Promotion::get(info.level);
		if(promotion_data){
			discount = promotion_data->immediate_discount;
		}
	}

	const double original_unit_price = GlobalStatus::get(GlobalStatus::SLOT_ACC_CARD_UNIT_PRICE);
	const std::uint64_t card_unit_price = std::ceil(original_unit_price * discount - 0.001);
	const auto balance_to_consume = checked_mul(card_unit_price, cards_to_buy);
	ret[sslit("balanceToConsume")] = balance_to_consume;

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(info.account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCELERATION_CARDS, cards_to_buy,
		Events::ItemChanged::R_BUY_MORE_CARDS, info.account_id.get(), card_unit_price, 0, remarks);
	transaction.emplace_back(info.account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, balance_to_consume,
		Events::ItemChanged::R_BUY_MORE_CARDS, info.account_id.get(), card_unit_price, 0, remarks);
	transaction.emplace_back(info.account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_BALANCE_BUY_CARDS_HISTORICAL, balance_to_consume,
		Events::ItemChanged::R_BUY_MORE_CARDS, info.account_id.get(), card_unit_price, 0, remarks);
	const auto insufficient_item_id = ItemMap::commit_transaction_nothrow(transaction.data(), transaction.size());
	if(insufficient_item_id){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE;
		ret[sslit("errorMessage")] = "No enough account balance";
		return ret;
	}
	accumulate_balance_bonus(info.account_id, info.account_id, balance_to_consume, 0);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
