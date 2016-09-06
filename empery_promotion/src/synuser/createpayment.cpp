#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include "../mmain.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../item_ids.hpp"
#include "../item_transaction_element.hpp"
#include "../events/item.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/mysql/object_base.hpp>
#include <poseidon/mysql/exception.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>

namespace EmperyPromotion {

namespace {
#define MYSQL_OBJECT_NAME   Promotion_SynuserPayment
#define MYSQL_OBJECT_FIELDS \
	FIELD_STRING           (source)	\
	FIELD_STRING           (orderid)	\
	FIELD_STRING           (username)	\
	FIELD_BIGINT_UNSIGNED  (amount)	\
	FIELD_STRING           (currency)	\
	FIELD_DATETIME         (created_time)	\
	FIELD_TINYINT_UNSIGNED (state)
#include <poseidon/mysql/object_generator.hpp>
}

SYNUSER_SERVLET("createpayment", session, params){
	const auto &username = params.at("username");
	const auto &password = params.at("password");
	const auto &amount_str = params.at("amount");
	const auto &orderid = params.at("orderid");
	const auto &source = params.at("source");
	const auto &currency = params.at("currency");
	const auto &sign = params.at("sign");

	Poseidon::JsonObject root;

	const auto &md5_key = get_config<std::string>("synuser_md5key");
	const auto sign_md5 = Poseidon::md5_hash(username + password + amount_str + orderid + source + currency + md5_key);
	const auto sign_expected = Poseidon::Http::hex_encode(sign_md5.data(), sign_md5.size());
	if(sign != sign_expected){
		LOG_EMPERY_PROMOTION_WARNING("Unexpected sign from ", session->get_remote_info(),
			": sign_expected = ", sign_expected, ", sign = ", sign);
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "sign_invalid";
		return root;
	}

	auto info = AccountMap::get_by_login_name(username);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "user_not_found";
		return root;
	}
	if(password != info.deal_password_hash){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "pass_error";
		return root;
	}

	ItemId item_id;
	std::uint64_t amount = 0;
	const auto amount_raw = boost::lexical_cast<double>(amount_str);
	if(currency == "PAI"){
		item_id = ItemIds::ID_ACCOUNT_BALANCE;
		amount = static_cast<std::uint64_t>(std::ceil(amount_raw * 100 - 0.001));
	} else if(currency == "USD"){
		item_id = ItemIds::ID_USD;
		amount = static_cast<std::uint64_t>(std::ceil(amount_raw * 100 - 0.001));
	} else if(currency == "ACC"){
		item_id = ItemIds::ID_ACCELERATION_CARDS;
		amount = static_cast<std::uint64_t>(std::ceil(amount_raw - 0.001));
	}
	if(!item_id){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "currency_not_supported";
		return root;
	}

	const auto utc_now = Poseidon::get_utc_time();
	const auto obj = boost::make_shared<Promotion_SynuserPayment>(source, orderid, username, amount, currency, utc_now, false);
	obj->enable_auto_saving();
	const auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);
	try {
		Poseidon::JobDispatcher::yield(promise, false);
	} catch(Poseidon::MySql::Exception &e){
		LOG_POSEIDON_WARNING("Poseidon::MySqlException: code = ", e.get_code(), ", message = ", e.get_message());
		root[sslit("state")] = "failed";
		if(e.get_code() == 1062){ // ER_DUP_ENTRY
			root[sslit("errorcode")] = "duplicate_orderid";
		} else {
			root[sslit("errorcode")] = "database_error";
		}
		return root;
	}

	const auto elem = ItemTransactionElement(info.account_id, ItemTransactionElement::OP_REMOVE, item_id, amount,
		Events::ItemChanged::R_SYNUSER_PAYMENT, 0, 0, 0, orderid);
	const auto insuff_item_id = ItemMap::commit_transaction_nothrow(&elem, 1);
	if(insuff_item_id){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "balance_not_enough";
		return root;
	}
	obj->set_state(true);

	root[sslit("state")] = "success";
	return root;
}

}
