#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/err_account.hpp"
#include "../auction_transaction.hpp"
#include "../account.hpp"
#include "../singletons/auction_transaction_map.hpp"
#include "../singletons/account_map.hpp"
#include "../checked_arithmetic.hpp"
#include "../item_ids.hpp"
#include "../data/item.hpp"

namespace EmperyCenter {

namespace {
	void fill_auction_transaction_object(Poseidon::JsonObject &object, const boost::shared_ptr<AuctionTransaction> &auction_transaction){
		PROFILE_ME;

		object[sslit("serial")]       = auction_transaction->get_serial();
		object[sslit("account_uuid")] = auction_transaction->get_account_uuid().str();
		object[sslit("operation")]    = static_cast<unsigned>(auction_transaction->get_operation());
		object[sslit("created_time")] = auction_transaction->get_created_time();
		object[sslit("expiry_time")]  = auction_transaction->get_expiry_time();
		object[sslit("item_id")]      = auction_transaction->get_item_id().get();
		object[sslit("item_count")]   = auction_transaction->get_item_count();
	}
}

AUCTION_SERVLET("create_bill", root, session, params){
	const auto platform_id = boost::lexical_cast<PlatformId>(params.at("platform_id"));
	const auto &login_name = params.at("login_name");
	const auto operation   = static_cast<AuctionTransaction::Operation>(boost::lexical_cast<unsigned>(params.at("operation")));
	const auto item_id     = boost::lexical_cast<ItemId>(params.at("item_id"));
	const auto item_count  = boost::lexical_cast<std::uint64_t>(params.at("item_count"));
	const auto &remarks    = params.get("remarks");

	const auto account = AccountMap::get_or_reload_by_login_name(platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	const auto account_uuid = account->get_account_uuid();

	if(item_count == 0){
		return Response(Msg::ERR_ZERO_AUCTION_ITEM_COUNT);
	}

	const auto expiry_duration        = get_config<std::uint64_t> ("auction_transaction_expiry_duration", 432000000);
	const auto max_number_per_account = get_config<std::size_t>   ("auction_transaction_max_number_per_account", 30);

	std::vector<boost::shared_ptr<AuctionTransaction>> existent_bills;
	AuctionTransactionMap::get_by_account(existent_bills, account_uuid);
	const auto count_to_erase = saturated_sub(existent_bills.size(), max_number_per_account);
	for(std::size_t i = 0; i < count_to_erase; ++i){
		const auto &auction_transaction = existent_bills.at(i);
		LOG_EMPERY_CENTER_WARNING("Erasing auction transaction: account_uuid = ", account_uuid,
			", serial = ", auction_transaction->get_serial());
		if(!auction_transaction->has_been_committed() && !auction_transaction->has_been_cancelled()){
			auction_transaction->cancel("Max number of bills per account exceeded");
		}
	}

	auto serial = AuctionTransaction::random_serial();
	const auto utc_now = Poseidon::get_utc_time();
	auto auction_transaction = boost::make_shared<AuctionTransaction>(std::move(serial),
		account_uuid, operation, utc_now, saturated_add(utc_now, expiry_duration), item_id, item_count, remarks);
	AuctionTransactionMap::insert(auction_transaction);

	Poseidon::JsonObject object;
	fill_auction_transaction_object(object, auction_transaction);
	root[sslit("auction_transaction")] = std::move(object);

	return Response();
}

}
