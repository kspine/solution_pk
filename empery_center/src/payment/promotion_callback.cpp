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
#include "../string_utilities.hpp"
#include "../account_utilities.hpp"
#include "../data/item.hpp"

namespace EmperyCenter {

namespace {
	boost::container::flat_map<unsigned, std::uint64_t> g_level_prices;

	MODULE_RAII_PRIORITY(/* handles */, 1000){
		Poseidon::CsvParser csv;

		boost::container::flat_map<unsigned, std::uint64_t> level_prices;
		level_prices.reserve(20);
		csv.load("empery_promotion_levels.csv");
		while(csv.fetch_row()){
			unsigned level = 0;
			std::uint64_t price = 0;

			csv.get(level, "displayLevel");
			csv.get(price, "immediatePrice");

			if(!level_prices.emplace(level, price).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate promotion level: level = ", level);
				DEBUG_THROW(Exception, sslit("Duplicate promotion level"));
			}
		}
		g_level_prices = std::move(level_prices);
	}
}

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

	if(payment_transaction->has_been_committed()){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_COMMITTED);
	}

	const auto item_id = payment_transaction->get_item_id();
	const auto item_data = Data::Item::require(item_id);

	std::uint64_t taxing_amount = 0;
	if(item_data->type.first == Data::Item::CAT_GIFT_BOX){
		const auto level = item_data->type.second;
		LOG_EMPERY_CENTER_DEBUG("Gift box: item_id = ", item_id, ", level = ", level, ", item_count = ", item_count);
		const auto unit_price = g_level_prices.at(level);
		taxing_amount = checked_mul(unit_price, item_count);
	}

	payment_transaction->commit(remarks);
	try {
		accumulate_promotion_bonus(payment_transaction->get_account_uuid(), taxing_amount);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_DEBUG("std::exception thrown: what = ", e.what());
	}

	return Response();
}

}
