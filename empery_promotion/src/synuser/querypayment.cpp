#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/mysql/connection.hpp>
#include <poseidon/mysql/utilities.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../mmain.hpp"
#include "../singletons/account_map.hpp"

namespace EmperyPromotion {

SYNUSER_SERVLET("querypayment", session, params){
	const auto &orderid = params.at("orderid");
	const auto &sign = params.at("sign");

	Poseidon::JsonObject root;

	const auto &md5_key = get_config<std::string>("synuser_md5key");
	const auto sign_md5 = Poseidon::md5_hash(orderid + md5_key);
	const auto sign_expected = Poseidon::Http::hex_encode(sign_md5.data(), sign_md5.size());
	if(sign != sign_expected){
		LOG_EMPERY_PROMOTION_WARNING("Unexpected sign from ", session->get_remote_info(),
			": sign_expected = ", sign_expected, ", sign = ", sign);
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "sign_invalid";
		return root;
	}

	struct Payment {
		std::string username;
		std::uint64_t amount;
		std::string currency;
		bool state;
	};
	const auto payment = boost::make_shared<Payment>();
	std::ostringstream oss;
	oss <<"SELECT * FROM `Promotion_SynuserPayment` WHERE `orderid` = " <<Poseidon::MySql::StringEscaper(orderid) <<" LIMIT 1";
	const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
		[=](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
			payment->username = conn->get_string("username");
			payment->amount   = conn->get_unsigned("amount");
			payment->currency = conn->get_string("currency");
			payment->state    = conn->get_unsigned("state");
		}, "Promotion_SynuserPayment", oss.str());
	Poseidon::JobDispatcher::yield(promise, true);

	auto info = AccountMap::get_by_login_name(payment->username);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "user_not_found";
		return root;
	}

	Poseidon::JsonObject data;
	data[sslit("username")] = std::move(payment->username);
	char str[64];
	if(payment->currency == "ACC"){
		std::sprintf(str, "%llu", (unsigned long long)payment->amount);
	} else {
		std::sprintf(str, "%.2f", payment->amount / 100.0);
	}
	data[sslit("amount")] = str;
	data[sslit("currency")] = std::move(payment->currency);
	data[sslit("sourceorderid")] = orderid;
	data[sslit("orderid")] = "";
	data[sslit("status")] = payment->state ? "success" : "failed";

	root[sslit("state")] = "success";
	root[sslit("data")] = std::move(data);
	return root;
}

}
