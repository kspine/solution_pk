#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../auction_transaction.hpp"
#include "../singletons/auction_transaction_map.hpp"

namespace EmperyCenter {

AUCTION_SERVLET("commit_bill", root, session, params){
	const auto &serial   = params.at("serial");
	const auto &remarks  = params.get("remarks");

	const auto auction_transaction = AuctionTransactionMap::get(serial);
	if(!auction_transaction){
		return Response(Msg::ERR_AUCTION_TRANSACTION_NOT_FOUND) <<serial;
	}
	if(auction_transaction->has_been_cancelled()){
		return Response(Msg::ERR_AUCTION_TRANSACTION_CANCELLED) <<serial;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(auction_transaction->get_expiry_time() < utc_now){
		return Response(Msg::ERR_AUCTION_TRANSACTION_EXPIRED) <<serial;
	}

	if(auction_transaction->has_been_committed()){
		return Response(Msg::ERR_AUCTION_TRANSACTION_COMMITTED);
	}

	auction_transaction->commit(remarks);

	return Response();
}

}
