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
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("rollbackWithdrawal", session, params){
	const auto &serial = params.at("serial");
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;

	std::vector<boost::shared_ptr<MySql::Promotion_WdSlip>> objs;
	std::ostringstream oss;
	oss <<"SELECT * FROM `Promotion_WdSlip` WHERE `serial` = " <<Poseidon::MySql::StringEscaper(serial) <<" LIMIT 0, 1";
	auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
		[&](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
			auto obj = boost::make_shared<MySql::Promotion_WdSlip>();
			obj->fetch(conn);
			objs.emplace_back(std::move(obj));
		}, "Promotion_WdSlip", oss.str());
	Poseidon::JobDispatcher::yield(promise, true);
	if(objs.empty()){
		ret[sslit("errorCode")] = (int)Msg::ERR_W_D_SLIP_NOT_FOUND;
		ret[sslit("errorMessage")] = "Withdrawal slip is not found";
		return ret;
	}
	const auto wd_slip_obj = std::move(objs.front());

	if(wd_slip_obj->get_state() == (unsigned)BillStates::ST_SETTLED){
		ret[sslit("errorCode")] = (int)Msg::ERR_W_D_SLIP_SETTLED;
		ret[sslit("errorMessage")] = "Withdrawal slip is already settled";
		return ret;
	}
	if(wd_slip_obj->get_state() == (unsigned)BillStates::ST_CANCELLED){
		ret[sslit("errorCode")] = (int)Msg::ERR_W_D_SLIP_CANCELLED;
		ret[sslit("errorMessage")] = "Withdrawal slip is cancelled";
		return ret;
	}

	std::string callback_ip(session->get_remote_info().ip.get());

	const auto amount = wd_slip_obj->get_amount();
	const auto delta_balance = amount + wd_slip_obj->get_fee();

	const auto account_id = AccountId(wd_slip_obj->get_account_id());
	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_WITHDRAWN_BALANCE, amount,
		Events::ItemChanged::R_ROLLBACK_WITHDRAWAL, account_id.get(), 0, 0, remarks);
	transaction.emplace_back(account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, delta_balance,
		Events::ItemChanged::R_ROLLBACK_WITHDRAWAL, account_id.get(), 0, 0, remarks);
	ItemMap::commit_transaction(transaction.data(), transaction.size());

	wd_slip_obj->set_state(BillStates::ST_CANCELLED);
	wd_slip_obj->set_callback_ip(std::move(callback_ip));

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
