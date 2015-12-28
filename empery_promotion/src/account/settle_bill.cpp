#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/item_map.hpp"
#include "../item_transaction_element.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"
#include "../mysql/bill.hpp"
#include "../bill_states.hpp"
#include <poseidon/mysql/utilities.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("settleBill", session, params){
	const auto &serial = params.at("serial");
	const auto amount = boost::lexical_cast<std::uint64_t>(params.at("amount"));

	Poseidon::JsonObject ret;

	std::vector<boost::shared_ptr<MySql::Promotion_Bill>> objs;
	std::ostringstream oss;
	oss <<"SELECT * FROM `Promotion_Bill` WHERE `serial` = '" <<Poseidon::MySql::StringEscaper(serial) <<"' LIMIT 0, 1";
	MySql::Promotion_Bill::batch_load(objs, oss.str());
	if(objs.empty()){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_BILL;
		ret[sslit("errorMessage")] = "Bill is not found";
		return ret;
	}
	const auto bill_obj = std::move(objs.front());

	if(bill_obj->get_state() == (unsigned)BillStates::ST_CANCELLED){
		ret[sslit("errorCode")] = (int)Msg::ERR_BILL_CANCELLED;
		ret[sslit("errorMessage")] = "Bill is cancelled";
		return ret;
	}
	if(bill_obj->get_state() == (unsigned)BillStates::ST_SETTLED){
		ret[sslit("errorCode")] = (int)Msg::ERR_BILL_SETTLED;
		ret[sslit("errorMessage")] = "Bill is already settled";
		return ret;
	}
	if(amount != bill_obj->get_amount()){
		ret[sslit("errorCode")] = (int)Msg::ERR_BILL_AMOUNT_MISMATCH;
		ret[sslit("errorMessage")] = "Amount mismatch";
		return ret;
	}

	std::string callback_ip(session->get_remote_info().ip.get());

	const auto account_id = AccountId(bill_obj->get_account_id());
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, bill_obj->get_amount(),
		Events::ItemChanged::R_RECHARGE, bill_obj->get_account_id(), 0, 0, serial);
	transaction.emplace_back(account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_BALANCE_RECHARGED_HISTORICAL, bill_obj->get_amount(),
		Events::ItemChanged::R_RECHARGE, bill_obj->get_account_id(), 0, 0, serial);
	ItemMap::commit_transaction(transaction.data(), transaction.size());

	bill_obj->set_state(BillStates::ST_SETTLED);
	bill_obj->set_callback_ip(std::move(callback_ip));

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
