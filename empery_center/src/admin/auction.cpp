#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/auction_transaction_map.hpp"
#include "../auction_transaction.hpp"

namespace EmperyCenter {

namespace {
	void fill_auction_transaction_object(Poseidon::JsonObject &object, const boost::shared_ptr<AuctionTransaction> &auction_transaction){
		PROFILE_ME;

		object[sslit("serial")]            = auction_transaction->get_serial();
		object[sslit("account_uuid")]      = auction_transaction->get_account_uuid().str();
		object[sslit("operation")]         = static_cast<unsigned>(auction_transaction->get_operation());
		object[sslit("created_time")]      = auction_transaction->get_created_time();
		object[sslit("expiry_time")]       = auction_transaction->get_expiry_time();
		object[sslit("item_id")]           = auction_transaction->get_item_id().get();
		object[sslit("item_count")]        = auction_transaction->get_item_count();
		object[sslit("remarks")]           = auction_transaction->get_remarks();
		object[sslit("committed")]         = auction_transaction->has_been_committed();
		object[sslit("cancelled")]         = auction_transaction->has_been_cancelled();
		object[sslit("operation_remarks")] = auction_transaction->get_operation_remarks();
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

	auction_transaction->commit(remarks);

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

	auction_transaction->cancel(remarks);

	return Response();
}

}
