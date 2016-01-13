#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/err_account.hpp"
#include "../payment_transaction.hpp"
#include "../account.hpp"
#include "../singletons/payment_transaction_map.hpp"
#include "../singletons/account_map.hpp"
#include "../checked_arithmetic.hpp"
#include "../item_ids.hpp"
#include "../data/item.hpp"

namespace EmperyCenter {

namespace {
	void fill_payment_transaction_object(Poseidon::JsonObject &object, const boost::shared_ptr<PaymentTransaction> &payment_transaction){
		PROFILE_ME;

		object[sslit("serial")]       = payment_transaction->get_serial();
		object[sslit("account_uuid")] = payment_transaction->get_account_uuid().str();
		object[sslit("created_time")] = payment_transaction->get_created_time();
		object[sslit("expiry_time")]  = payment_transaction->get_expiry_time();
		object[sslit("item_id")]      = payment_transaction->get_item_id().get();
		object[sslit("item_count")]   = payment_transaction->get_item_count();
	}

	boost::shared_ptr<PaymentTransaction> really_create_bill(
		AccountUuid account_uuid, ItemId item_id, std::uint64_t amount, const std::string &remarks)
	{
		PROFILE_ME;

		const auto expiry_duration        = get_config<std::uint64_t> ("payment_transaction_expiry_duration", 432000000);
		const auto max_number_per_account = get_config<std::size_t>   ("payment_transaction_max_number_per_account", 30);

		std::vector<boost::shared_ptr<PaymentTransaction>> existent_bills;
		PaymentTransactionMap::get_by_account(existent_bills, account_uuid);
		const auto count_to_erase = saturated_sub(existent_bills.size(), max_number_per_account);
		for(std::size_t i = 0; i < count_to_erase; ++i){
			const auto &payment_transaction = existent_bills.at(i);
			LOG_EMPERY_CENTER_WARNING("Erasing payment transaction: account_uuid = ", account_uuid,
				", serial = ", payment_transaction->get_serial());
			if(!payment_transaction->is_virtually_removed()){
				payment_transaction->cancel("Max number of bills per account exceeded");
			}
		}

		auto serial = PaymentTransaction::random_serial();
		const auto utc_now = Poseidon::get_utc_time();
		auto payment_transaction = boost::make_shared<PaymentTransaction>(std::move(serial),
			account_uuid, utc_now, saturated_add(utc_now, expiry_duration), item_id, amount, remarks);
		PaymentTransactionMap::insert(payment_transaction);

		return payment_transaction;
	}
}

PAYMENT_SERVLET("create_bill/diamonds", root, session, params){
	const auto platform_id = boost::lexical_cast<PlatformId>(params.at("platform_id"));
	const auto &login_name = params.at("login_name");
	const auto amount      = boost::lexical_cast<std::uint64_t>(params.at("amount"));
	const auto &remarks    = params.get("remarks");

	const auto account = AccountMap::get_by_login_name(platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	if(amount == 0){
		return Response(Msg::ERR_ZERO_PAYMENT_AMOUNT);
	}

	const auto payment_transaction = really_create_bill(account->get_account_uuid(), ItemIds::ID_DIAMONDS, amount, remarks);

	Poseidon::JsonObject object;
	fill_payment_transaction_object(object, payment_transaction);
	root[sslit("payment_transaction")] = std::move(object);

	return Response();
}

PAYMENT_SERVLET("create_bill/alternative_gold_coins", root, session, params){
	const auto platform_id = boost::lexical_cast<PlatformId>(params.at("platform_id"));
	const auto &login_name = params.at("login_name");
	const auto amount      = boost::lexical_cast<std::uint64_t>(params.at("amount"));
	const auto &remarks    = params.get("remarks");

	const auto account = AccountMap::get_by_login_name(platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	if(amount == 0){
		return Response(Msg::ERR_ZERO_PAYMENT_AMOUNT);
	}

	const auto payment_transaction = really_create_bill(account->get_account_uuid(), ItemIds::ID_GOLD_COINS, amount, remarks);

	Poseidon::JsonObject object;
	fill_payment_transaction_object(object, payment_transaction);
	root[sslit("payment_transaction")] = std::move(object);

	return Response();
}

PAYMENT_SERVLET("create_bill/alternative_gift_box", root, session, params){
	const auto platform_id     = boost::lexical_cast<PlatformId>(params.at("platform_id"));
	const auto &login_name     = params.at("login_name");
	const auto promotion_level = boost::lexical_cast<unsigned>(params.at("promotion_level"));
	const auto &remarks        = params.get("remarks");

	const auto account = AccountMap::get_by_login_name(platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	const auto item_data = Data::Item::get_by_type(Data::Item::CAT_GIFT_BOX, promotion_level);
	if(!item_data){
		return Response(Msg::ERR_NO_SUCH_PROMOTION_LEVEL);
	}

	const auto payment_transaction = really_create_bill(account->get_account_uuid(), item_data->item_id, 1, remarks);

	Poseidon::JsonObject object;
	fill_payment_transaction_object(object, payment_transaction);
	root[sslit("payment_transaction")] = std::move(object);

	return Response();
}

}
