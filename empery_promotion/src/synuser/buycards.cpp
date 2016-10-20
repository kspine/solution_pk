#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include "../mmain.hpp"
#include "../singletons/account_map.hpp"
#include "../utilities.hpp"

namespace EmperyPromotion {

SYNUSER_SERVLET("buycards", session, params){
	const auto &username = params.at("username");
	const auto &cards_str = params.at("cards");
	const auto cards = boost::lexical_cast<std::uint64_t>(cards_str);
	const auto &sign = params.get("sign");

	Poseidon::JsonObject root;

	const auto &md5_key = get_config<std::string>("synuser_md5key");
	const auto sign_md5 = Poseidon::md5_hash(username + cards_str + md5_key);
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

	constexpr std::uint64_t unit_price = 50000;
	sell_acceleration_cards_mojinpai(info.account_id, unit_price, cards);

	root[sslit("state")] = "success";
	return root;
}

}
