#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/payment_transaction_map.hpp"
#include "../payment_transaction.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../singletons/item_box_map.hpp"
#include "../singletons/mail_box_map.hpp"

namespace EmperyCenter {

namespace {
	void fill_payment_transaction_object(Poseidon::JsonObject &object, const boost::shared_ptr<PaymentTransaction> &transaction){
		PROFILE_ME;

		const auto account = AccountMap::require(transaction->get_account_uuid());

		object[sslit("serial")]            = transaction->get_serial();
		object[sslit("account_uuid")]      = transaction->get_account_uuid().str();
		object[sslit("created_time")]      = transaction->get_created_time();
		object[sslit("expiry_time")]       = transaction->get_expiry_time();
		object[sslit("item_id")]           = transaction->get_item_id().get();
		object[sslit("item_count")]        = transaction->get_item_count();
		object[sslit("remarks")]           = transaction->get_remarks();
		object[sslit("committed")]         = transaction->has_been_committed();
		object[sslit("cancelled")]         = transaction->has_been_cancelled();
		object[sslit("operation_remarks")] = transaction->get_operation_remarks();

		object[sslit("platform_id")]       = account->get_platform_id().get();
		object[sslit("login_name")]        = account->get_login_name();
		object[sslit("nick")]              = account->get_nick();
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

	const auto item_box = ItemBoxMap::require(payment_transaction->get_account_uuid());
	const auto mail_box = MailBoxMap::require(payment_transaction->get_account_uuid());

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

	payment_transaction->commit(item_box, mail_box, remarks);

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
