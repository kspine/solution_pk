#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include "../mmain.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"

namespace EmperyPromotion {

SYNUSER_SERVLET("setDealPassword", session, params){
	const auto &login_name = params.at("loginName");
	const auto &deal_password = params.at("dealPassword");
	auto new_deal_password = params.at("newDealPassword");
	const auto &sign = params.at("sign");

	Poseidon::JsonObject root;

	const auto &md5_key = get_config<std::string>("synuser_md5key");
	const auto sign_md5 = Poseidon::md5_hash(login_name + deal_password + new_deal_password + md5_key);
	const auto sign_expected = Poseidon::Http::hex_encode(sign_md5.data(), sign_md5.size());
	if(sign != sign_expected){
		LOG_EMPERY_PROMOTION_WARNING("Unexpected sign from ", session->get_remote_info(),
			": sign_expected = ", sign_expected, ", sign = ", sign);
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "sign_invalid";
		return root;
	}

	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "user_not_found";
		return root;
	}
	if(deal_password != info.deal_password_hash){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "deal_password_incorrect";
		return root;
	}
	if((info.banned_until != 0) && (Poseidon::get_utc_time() < info.banned_until)){
		root[sslit("state")] = "failed";
		root[sslit("errorcode")] = "account_banned";
		return root;
	}

	AccountMap::set_deal_password(info.account_id, std::move(new_deal_password));

	root[sslit("state")] = "success";
	return root;
}

}
