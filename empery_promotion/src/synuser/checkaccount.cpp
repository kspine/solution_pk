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
	const auto &phone_number = params.get("mobilePhone");
	const auto &sign = params.get("sign");

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

	std::vector<AccountMap::AccountInfo> accounts;
	accounts.reserve(16);
	auto info = AccountMap::get_by_login_name(username);
	if(Poseidon::has_any_flags_of(info.flags, AccountMap::FL_VALID)){
		accounts.emplace_back(std::move(info));
	}
	AccountMap::get_by_phone_number(accounts, phone_number);

	Poseidon::JsonArray data;
	for(auto it = accounts.begin(); it != accounts.end(); ++it){
		Poseidon::JsonObject elem;
		elem[sslit("username")] = std::move(it->login_name);
		elem[sslit("password")] = std::move(it->password_hash);
		const auto utc_now = Poseidon::get_utc_time();
		elem[sslit("state")] = (it->banned_until < utc_now) ? "1" : "2";
		elem[sslit("source")] = "";
		char str[64];
		std::sprintf(str, "%.2f", ItemMap::get_count(it->account_id, ItemIds::ID_USD) / 100.0);
		elem[sslit("usdbalance")] = str;
		std::sprintf(str, "%.2f", ItemMap::get_count(it->account_id, ItemIds::ID_ACCOUNT_BALANCE) / 100.0);
		elem[sslit("paibalance")] = str;
		const auto level_elem = Data::Promotion::require(it->level);
		std::sprintf(str, "%u", level_elem->display_level);
		elem[sslit("level")] = str;
		info = AccountMap::get(info.referrer_id);
		if(Poseidon::has_any_flags_of(info.flags, AccountMap::FL_VALID)){
			elem[sslit("invite")] = std::move(info.login_name);
		}
		elem[sslit("invite")] = std::move(it->password_hash);
		elem[sslit("tradePassword")] = std::move(it->deal_password_hash);
		elem[sslit("mobilePhone")] = std::move(it->phone_number);
		data.emplace_back(std::move(elem));
	}

	root[sslit("state")] = "success";
	root[sslit("data")] = std::move(data);
	return root;
}

}
