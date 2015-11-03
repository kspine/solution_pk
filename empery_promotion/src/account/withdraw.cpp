#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/item_map.hpp"
#include "../item_transaction_element.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"
#include "../utilities.hpp"
#include "../mysql/wd_slip.hpp"
#include "../bill_states.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("withdraw", /* session */, params){
	const auto &loginName = params.at("loginName");
	const auto deltaBalance = boost::lexical_cast<boost::uint64_t>(params.at("deltaBalance"));
	const auto &dealPassword = params.at("dealPassword");
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;
	auto info = AccountMap::getByLoginName(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	if(AccountMap::getPasswordHash(dealPassword) != info.dealPasswordHash){
		ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_DEAL_PASSWORD;
		ret[sslit("errorMessage")] = "Deal password is incorrect";
		return ret;
	}
	if((info.bannedUntil != 0) && (Poseidon::getUtcTime() < info.bannedUntil)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Account is banned";
		return ret;
	}

	const auto wdSlipSerialPrefix = getConfig<std::string>("wd_slip_serial_prefix", "PKWD");
	const double withdrawalFeeRatio = getConfig<double>("withdrawal_fee_ratio", 0.05);

	const auto serial = generateBillSerial(wdSlipSerialPrefix);
	const auto localNow = Poseidon::getUtcTime();

	auto amount = deltaBalance;
	auto fee = static_cast<boost::uint64_t>(std::ceil(deltaBalance * withdrawalFeeRatio));
	if(fee > amount){
		fee = amount;
	}
	amount -= fee;

	const auto wdSlipObj = boost::make_shared<MySql::Promotion_WdSlip>(
		serial, localNow, amount, fee, info.accountId.get(), BillStates::ST_NEW, std::string(), remarks);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(info.accountId, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, deltaBalance,
		Events::ItemChanged::R_WITHDRAW, info.accountId.get(), 0, 0, remarks);
	transaction.emplace_back(info.accountId,ItemTransactionElement::OP_ADD, ItemIds::ID_WITHDRAWN_BALANCE, amount,
		Events::ItemChanged::R_WITHDRAW, info.accountId.get(), 0, 0, remarks);
	const auto insufficientItemId = ItemMap::commitTransactionNoThrow(transaction.data(), transaction.size());
	if(insufficientItemId){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE;
		ret[sslit("errorMessage")] = "No enough account balance";
		return ret;
	}

	LOG_EMPERY_PROMOTION_INFO("Created withdraw slip: serial = ", serial,
		", amount = ", amount, ", fee = ", fee, ", accountId = ", info.accountId, ", remarks = ", remarks);
	wdSlipObj->asyncSave(true);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
