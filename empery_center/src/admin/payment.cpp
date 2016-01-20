#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/payment_transaction_map.hpp"
#include "../payment_transaction.hpp"

namespace EmperyCenter {

namespace {
	void fill_payment_transaction_object(Poseidon::JsonObject &object, const boost::shared_ptr<PaymentTransaction> &payment_transaction){
		PROFILE_ME;

		object[sslit("serial")]            = payment_transaction->get_serial();
		object[sslit("account_uuid")]      = payment_transaction->get_account_uuid().str();
		object[sslit("created_time")]      = payment_transaction->get_created_time();
		object[sslit("expiry_time")]       = payment_transaction->get_expiry_time();
		object[sslit("item_id")]           = payment_transaction->get_item_id();
		object[sslit("item_count")]        = payment_transaction->get_item_count();
		object[sslit("remarks")]           = payment_transaction->get_remarks();
		object[sslit("committed")]         = payment_transaction->has_been_committed();
		object[sslit("cancelled")]         = payment_transaction->has_been_cancelled();
		object[sslit("operation_remarks")] = payment_transaction->get_operation_remarks();
	}
}

ADMIN_SERVLET("payment/get_transaction", root, session, params){
	const auto &serial = params.at("serial");

	const auto payment_transaction = PaymentTransactionMap::get(serial);
	if(!payment_transaction){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_NOT_FOUND) <<serial;
	}

	Poseidon::JsonObject object;
	fill_payment_transaction_object(object, payment_transaction);
	root[sslit("payment_transaction")] = std::move(object);

	return Response();
}

ADMIN_SERVLET("payment/commit_transaction", root, session, params){
	const auto &serial  = params.at("serial");
	const auto &remarks = params.get("remarks");

	const auto payment_transaction = PaymentTransactionMap::get(serial);
	if(!payment_transaction){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_NOT_FOUND) <<serial;
	}
	if(payment_transaction->has_been_committed()){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_COMMITTED) <<serial;
	}
	if(payment_transaction->has_been_cancelled()){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_CANCELLED) <<serial;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(payment_transaction->get_expiry_time() < utc_now){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_EXPIRED) <<serial;
	}

	payment_transaction->commit(remarks);

	return Response();
}

ADMIN_SERVLET("payment/cancel_transaction", root, session, params){
	const auto &serial  = params.at("serial");
	const auto &remarks = params.get("remarks");

	const auto payment_transaction = PaymentTransactionMap::get(serial);
	if(!payment_transaction){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_NOT_FOUND) <<serial;
	}
	if(payment_transaction->has_been_committed()){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_COMMITTED) <<serial;
	}
	if(payment_transaction->has_been_cancelled()){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_CANCELLED) <<serial;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(payment_transaction->get_expiry_time() < utc_now){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_EXPIRED) <<serial;
	}

	payment_transaction->cancel(remarks);

	return Response();
}

}
