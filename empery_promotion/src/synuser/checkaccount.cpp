#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include "../mmain.hpp"
#include "../singletons/account_map.hpp"
#include "../data/promotion.hpp"
#include "../singletons/item_map.hpp"
#include "../item_ids.hpp"

namespace EmperyPromotion {

SYNUSER_SERVLET("checkaccount", session, params){
	const auto &username = params.at("username");
	const auto &sign = params.at("sign");

	Poseidon::JsonObject root;

	const auto &md5_key = get_config<std::string>("synuser_md5key");
	const auto sign_md5 = Poseidon::md5_hash(username + md5_key);
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

	Poseidon::JsonObject data;
	data[sslit("username")] = std::move(info.login_name);
	data[sslit("password")] = std::move(info.password_hash);
	const auto utc_now = Poseidon::get_utc_time();
	data[sslit("state")] = (info.banned_until < utc_now) ? "1" : "2";
	data[sslit("source")] = "";
	char str[64];
	std::sprintf(str, "%.2f", ItemMap::get_count(info.account_id, ItemIds::ID_USD) / 100.0);
	data[sslit("usdbalance")] = str;
	std::sprintf(str, "%.2f", ItemMap::get_count(info.account_id, ItemIds::ID_ACCOUNT_BALANCE) / 100.0);
	data[sslit("paibalance")] = str;
	const auto level_data = Data::Promotion::require(info.level);
	std::sprintf(str, "%u", level_data->display_level);
	data[sslit("level")] = str;

	root[sslit("state")] = "success";
	root[sslit("data")] = std::move(data);
	return root;
}

}
