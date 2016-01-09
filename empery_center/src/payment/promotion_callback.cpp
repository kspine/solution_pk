#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include "../msg/err_account.hpp"
#include "../payment_transaction.hpp"
#include "../singletons/payment_transaction_map.hpp"
#include "../string_utilities.hpp"

namespace EmperyCenter {

PAYMENT_SERVLET("promotion_callback", root, session, params){
	const auto &serial   = params.at("serial");
	const auto amount    = boost::lexical_cast<std::uint64_t>(params.at("amount"));
	const auto &checksum = params.at("checksum");
	const auto &remarks  = params.get("remarks");

	const auto payment_transaction = PaymentTransactionMap::get(serial);
	if(!payment_transaction){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_NOT_FOUND) <<serial;
	}
	if(payment_transaction->has_been_cancelled()){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_CANCELLED) <<serial;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(payment_transaction->get_expiry_time() < utc_now){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_EXPIRED) <<serial;
	}

	const auto magic_key = get_config<std::string>("payment_promotion_magic_key");

	std::ostringstream oss;
	oss <<serial <<',' <<amount <<',' <<magic_key;
	const auto sha1 = Poseidon::sha1_hash(oss.str());
	const auto checksum_expected = Poseidon::Http::hex_encode(sha1.data(), sha1.size());
	if(checksum != checksum_expected){
		LOG_EMPERY_CENTER_DEBUG("Checksum mismatch: expecting = ", checksum_expected, ", got ", checksum);
		return Response(Msg::ERR_PAYMENT_CHECKSUM_ERROR);
	}

	if(payment_transaction->has_been_committed()){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_COMMITTED);
	}

	payment_transaction->settle(remarks);

	return Response();
}

}
