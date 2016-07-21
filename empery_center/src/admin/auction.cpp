#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/auction_transaction_map.hpp"
#include "../auction_transaction.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../singletons/item_box_map.hpp"
#include "../singletons/mail_box_map.hpp"
#include "../singletons/auction_center_map.hpp"
#include "../account_utilities.hpp"

namespace EmperyCenter {

namespace {
	void fill_auction_transaction_object(Poseidon::JsonObject &object, const boost::shared_ptr<AuctionTransaction> &transaction){
		PROFILE_ME;

		const auto account = AccountMap::require(transaction->get_account_uuid());

		object[sslit("serial")]            = transaction->get_serial();
		object[sslit("account_uuid")]      = transaction->get_account_uuid().str();
		object[sslit("operation")]         = static_cast<unsigned>(transaction->get_operation());
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

ADMIN_SERVLET("auction/get_transaction", root, session, params){
	const auto &serial = params.at("serial");

	const auto auction_transaction = AuctionTransactionMap::get(serial);
	if(!auction_transaction){
		return Response(Msg::ERR_AUCTION_TRANSACTION_NOT_FOUND) <<serial;
	}

	Poseidon::JsonObject object;
	fill_auction_transaction_object(object, auction_transaction);
	root[sslit("auction_transaction")] = std::move(object);

	return Response();
}

ADMIN_SERVLET("auction/commit_transaction", root, session, params){
	const auto &serial  = params.at("serial");
	const auto &remarks = params.get("remarks");

	const auto auction_transaction = AuctionTransactionMap::get(serial);
	if(!auction_transaction){
		return Response(Msg::ERR_AUCTION_TRANSACTION_NOT_FOUND) <<serial;
	}

	const auto account_uuid = auction_transaction->get_account_uuid();
	AccountMap::require_controller_token(account_uuid);

	const auto item_box = ItemBoxMap::require(account_uuid);
	const auto mail_box = MailBoxMap::require(account_uuid);
	const auto auction_center = AuctionCenterMap::require(account_uuid);

	if(auction_transaction->has_been_committed()){
		return Response(Msg::ERR_AUCTION_TRANSACTION_COMMITTED) <<serial;
	}
	if(auction_transaction->has_been_cancelled()){
		return Response(Msg::ERR_AUCTION_TRANSACTION_CANCELLED) <<serial;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(auction_transaction->get_expiry_time() < utc_now){
		return Response(Msg::ERR_AUCTION_TRANSACTION_EXPIRED) <<serial;
	}

	auction_transaction->commit(item_box, mail_box, auction_center, remarks);

	return Response();
}

ADMIN_SERVLET("auction/cancel_transaction", root, session, params){
	const auto &serial  = params.at("serial");
	const auto &remarks = params.get("remarks");

	const auto auction_transaction = AuctionTransactionMap::get(serial);
	if(!auction_transaction){
		return Response(Msg::ERR_AUCTION_TRANSACTION_NOT_FOUND) <<serial;
	}
	if(auction_transaction->has_been_committed()){
		return Response(Msg::ERR_AUCTION_TRANSACTION_COMMITTED) <<serial;
	}
	if(auction_transaction->has_been_cancelled()){
		return Response(Msg::ERR_AUCTION_TRANSACTION_CANCELLED) <<serial;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(auction_transaction->get_expiry_time() < utc_now){
		return Response(Msg::ERR_AUCTION_TRANSACTION_EXPIRED) <<serial;
	}

	const auto account_uuid = auction_transaction->get_account_uuid();
	AccountMap::require_controller_token(account_uuid);

	auction_transaction->cancel(remarks);

	return Response();
}

}
