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
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../data/map.hpp"
#include "../data/global.hpp"
#include "../buff_ids.hpp"
#include "../msg/err_castle.hpp"
#include "../data/item.hpp"

namespace EmperyCenter {

PAYMENT_SERVLET("promotion_callback", root, session, params){
	const auto &serial   = params.at("serial");
	const auto &checksum = params.at("checksum");
	const auto &remarks  = params.get("remarks");

	const auto payment_transaction = PaymentTransactionMap::get(serial);
	if(!payment_transaction){
		return Response(Msg::ERR_PAYMENT_TRANSACTION_NOT_FOUND) <<serial;
	}

	const auto account_uuid = payment_transaction->get_account_uuid();
	const auto account = AccountMap::get_or_reload(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}
	AccountMap::require_controller_token(account_uuid);

	const auto item_box = ItemBoxMap::require(account_uuid);
	const auto mail_box = MailBoxMap::require(account_uuid);

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
	const auto item_category = item_data->type.first;

	payment_transaction->commit(item_box, mail_box, remarks);

	if((item_category == Data::Item::CAT_LEVEL_GIFT_BOX) || (item_category == Data::Item::CAT_LARGE_LEVEL_GIFT_BOX)){
		auto primary_castle = WorldMap::get_primary_castle(account_uuid);
		if(!primary_castle){
			LOG_EMPERY_CENTER_INFO("Creating initial castle: account_uuid = ", account_uuid);
			primary_castle = WorldMap::place_castle_random(
				[&](Coord coord){
					const auto castle_uuid = MapObjectUuid(Poseidon::Uuid::random());
					const auto utc_now = Poseidon::get_utc_time();

					const auto protection_minutes = Data::Global::as_unsigned(Data::Global::SLOT_NOVICIATE_PROTECTION_DURATION);
					const auto protection_duration = saturated_mul<std::uint64_t>(protection_minutes, 60000);

					auto castle = boost::make_shared<Castle>(castle_uuid, account_uuid, MapObjectUuid(), account->get_nick(), coord, utc_now);
					castle->set_buff(BuffIds::ID_NOVICIATE_PROTECTION, utc_now, protection_duration);
					return castle;
				});
			if(!primary_castle){
				return Response(Msg::ERR_NO_START_POINTS_AVAILABLE);
			}
		}

		account->set_activated(true);
	}

	Poseidon::async_raise_event(
		boost::make_shared<Events::AccountSynchronizeWithThirdServer>(
			boost::shared_ptr<long>(), account->get_platform_id(), account->get_login_name(), std::string()));

	return Response();
}

}
