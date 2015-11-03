#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../data/promotion.hpp"
#include "../checked_arithmetic.hpp"
#include "../singletons/item_map.hpp"
#include "../singletons/global_status.hpp"
#include "../data/promotion.hpp"
#include "../events/item.hpp"
#include "../item_transaction_element.hpp"
#include "../item_ids.hpp"
#include "../utilities.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("buyMoreCards", /* session */, params){
	const auto &loginName = params.at("loginName");
	const auto cardsToBuy = boost::lexical_cast<boost::uint64_t>(params.at("cardsToBuy"));
	const auto &dealPassword = params.at("dealPassword");
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;

	auto info = AccountMap::getByLoginName(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_PAYER;
		ret[sslit("errorMessage")] = "Payer is not found";
		return ret;
	}
	if(AccountMap::getPasswordHash(dealPassword) != info.dealPasswordHash){
		ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_DEAL_PASSWORD;
		ret[sslit("errorMessage")] = "Deal password is incorrect";
		return ret;
	}
	const auto localNow = Poseidon::getLocalTime();
	if((info.bannedUntil != 0) && (localNow < info.bannedUntil)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Payer is banned";
		return ret;
	}

	double discount = 1.0;
	if(info.level != 0){
		const auto promotionData = Data::Promotion::get(info.level);
		if(promotionData){
			discount = promotionData->immediateDiscount;
		}
	}

	const double originalUnitPrice = GlobalStatus::get(GlobalStatus::SLOT_ACC_CARD_UNIT_PRICE);
	const boost::uint64_t cardUnitPrice = std::ceil(originalUnitPrice * discount);
	const auto balanceToConsume = checkedMul(cardUnitPrice, cardsToBuy);
	ret[sslit("balanceToConsume")] = balanceToConsume;

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(info.accountId, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCELERATION_CARDS, cardsToBuy,
		Events::ItemChanged::R_BUY_MORE_CARDS, info.accountId.get(), cardUnitPrice, 0, remarks);
	transaction.emplace_back(info.accountId, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, balanceToConsume,
		Events::ItemChanged::R_BUY_MORE_CARDS, info.accountId.get(), cardUnitPrice, 0, remarks);
	const auto insufficientItemId = ItemMap::commitTransactionNoThrow(transaction.data(), transaction.size());
	if(insufficientItemId){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE;
		ret[sslit("errorMessage")] = "No enough account balance";
		return ret;
	}
	accumulateBalanceBonus(info.accountId, info.accountId, balanceToConsume, 0);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
