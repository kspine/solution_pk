#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/item_map.hpp"
#include "../item_transaction_element.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"
#include "../mysql/wd_slip.hpp"
#include "../bill_states.hpp"
#include <poseidon/mysql/utilities.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("commitWithdrawal", session, params){
	const auto &serial = params.at("serial");
	const auto amount = boost::lexical_cast<boost::uint64_t>(params.at("amount"));

	Poseidon::JsonObject ret;

	std::vector<boost::shared_ptr<MySql::Promotion_WdSlip> > objs;
	std::ostringstream oss;
	oss <<"SELECT * FROM `Promotion_WdSlip` WHERE `serial` = '" <<Poseidon::MySql::StringEscaper(serial) <<"' LIMIT 0, 1";
	MySql::Promotion_WdSlip::batchLoad(objs, oss.str());
	if(objs.empty()){
		ret[sslit("errorCode")] = (int)Msg::ERR_W_D_SLIP_NOT_FOUND;
		ret[sslit("errorMessage")] = "Withdrawal slip is not found";
		return ret;
	}
	const auto wdSlipObj = std::move(objs.front());

	if(wdSlipObj->get_state() == (unsigned)BillStates::ST_CANCELLED){
		ret[sslit("errorCode")] = (int)Msg::ERR_W_D_SLIP_CANCELLED;
		ret[sslit("errorMessage")] = "Withdrawal slip is cancelled";
		return ret;
	}
	if(wdSlipObj->get_state() == (unsigned)BillStates::ST_SETTLED){
		ret[sslit("errorCode")] = (int)Msg::ERR_W_D_SLIP_SETTLED;
		ret[sslit("errorMessage")] = "Withdrawal slip is settled";
		return ret;
	}
	if(amount != wdSlipObj->get_amount()){
		ret[sslit("errorCode")] = (int)Msg::ERR_W_D_BALANCE_MISMATCH;
		ret[sslit("errorMessage")] = "Amount mismatch";
		return ret;
	}

	std::string callbackIp(session->getRemoteInfo().ip.get());

	const auto accountId = AccountId(wdSlipObj->get_accountId());
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(accountId,
		ItemTransactionElement::OP_REMOVE, ItemIds::ID_WITHDRAWN_BALANCE, amount);
	const auto insufficientItemId = ItemMap::commitTransactionNoThrow(transaction.data(), transaction.size(),
		Events::ItemChanged::R_COMMIT_WITHDRAWAL, accountId.get(), 0, 0, wdSlipObj->unlockedGet_remarks());
	if(insufficientItemId){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_W_D_BALANCE;
		ret[sslit("errorMessage")] = "No enough withdrawn balance";
		return ret;
	}

	wdSlipObj->set_state(BillStates::ST_SETTLED);
	wdSlipObj->set_callbackIp(std::move(callbackIp));

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
