#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/item_map.hpp"
#include "../item_transaction_element.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("transfer", session, params){
	const auto &loginName = params.at("loginName");
	const auto deltaBalance = boost::lexical_cast<boost::uint64_t>(params.at("deltaBalance"));
	const auto &dealPassword = params.at("dealPassword");
	const auto &toLoginName = params.at("toLoginName");
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;
	auto srcInfo = AccountMap::getByLoginName(loginName);
	if(Poseidon::hasNoneFlagsOf(srcInfo.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	if(AccountMap::getPasswordHash(dealPassword) != srcInfo.dealPasswordHash){
		ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_DEAL_PASSWORD;
		ret[sslit("errorMessage")] = "Deal password is incorrect";
		return ret;
	}
	if((srcInfo.bannedUntil != 0) && (Poseidon::getUtcTime() < srcInfo.bannedUntil)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Account is banned";
		return ret;
	}

	auto dstInfo = AccountMap::getByLoginName(toLoginName);
	if(Poseidon::hasNoneFlagsOf(dstInfo.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_TRANSFER_DEST_NOT_FOUND;
		ret[sslit("errorMessage")] = "Destination account is not found";
		return ret;
	}
	if((dstInfo.bannedUntil != 0) && (Poseidon::getUtcTime() < dstInfo.bannedUntil)){
		ret[sslit("errorCode")] = (int)Msg::ERR_TRANSFER_DEST_BANNED;
		ret[sslit("errorMessage")] = "Destination account is banned";
		return ret;
	}

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(srcInfo.accountId, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, deltaBalance,
		Events::ItemChanged::R_TRANSFER, srcInfo.accountId.get(), dstInfo.accountId.get(), 0, remarks);
	transaction.emplace_back(dstInfo.accountId, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, deltaBalance,
		Events::ItemChanged::R_TRANSFER, srcInfo.accountId.get(), dstInfo.accountId.get(), 0, remarks);
	const auto insufficientItemId = ItemMap::commitTransactionNoThrow(transaction.data(), transaction.size());
	if(insufficientItemId){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE;
		ret[sslit("errorMessage")] = "No enough account balance";
		return ret;
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
