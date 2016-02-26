#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/csv_parser.hpp>
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include "../msg/err_account.hpp"
#include "../payment_transaction.hpp"
#include "../singletons/payment_transaction_map.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_box_map.hpp"
#include "../singletons/mail_box_map.hpp"
#include "../account.hpp"
#include "../string_utilities.hpp"
#include "../events/account.hpp"

namespace EmperyCenter {

PAYMENT_SERVLET("promotion_callback", root, session, params){
	const auto &serial   = params.at("serial");
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

	const auto item_count = payment_transaction->get_item_count();

	std::ostringstream oss;
	oss <<serial <<',' <<item_count <<',' <<magic_key;
	const auto sha1 = Poseidon::sha1_hash(oss.str());
	const auto checksum_expected = Poseidon::Http::hex_encode(sha1.data(), sha1.size());
	if(checksum != checksum_expected){
		LOG_EMPERY_CENTER_DEBUG("Checksum mismatch: expecting = ", checksum_expected, ", got ", checksum);
		return Response(Msg::ERR_PAYMENT_CHECKSUM_ERROR);
	}

	const auto account = AccountMap::require(payment_transaction->get_account_uuid());
	const auto item_box = ItemBoxMap::require(payment_transaction->get_account_uuid());
	const auto mail_box = MailBoxMap::require(payment_transaction->get_account_uuid());

	if(payment_transaction->has_been_committed()){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_COMMITTED);
	}

	payment_transaction->commit(item_box, mail_box, remarks);

	Poseidon::async_raise_event(
		boost::make_shared<Events::AccountSynchronizeWithThirdServer>(
			boost::shared_ptr<long>(), account->get_platform_id(), account->get_login_name(), std::string()));

	return Response();
}

}
