#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("queryAccountAttributes", session, params){
	const auto &login_name = params.at("loginName");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	std::string referrer_login_name, referrer_nick;
	if(info.referrer_id){
		auto referrer_info = AccountMap::require(info.referrer_id);
		referrer_login_name = std::move(referrer_info.login_name);
		referrer_nick = std::move(referrer_info.nick);
	}

	boost::uint64_t max_visible_subord_depth;
	const auto &str = AccountMap::get_attribute(info.account_id, AccountMap::ATTR_MAX_VISIBLE_SUBORD_DEPTH);
	if(str.empty()){
		max_visible_subord_depth = get_config<boost::uint64_t>("default_max_visible_subord_depth", 2);
	} else {
		max_visible_subord_depth = boost::lexical_cast<boost::uint64_t>(str);
	}

	ret[sslit("phoneNumber")] = std::move(info.phone_number);
	ret[sslit("nick")] = std::move(info.nick);
	ret[sslit("level")] = boost::lexical_cast<std::string>(info.level);
	ret[sslit("referrerLoginName")] = std::move(referrer_login_name);
	ret[sslit("referrerNick")] = std::move(referrer_nick);
	ret[sslit("flags")] = info.flags;
	ret[sslit("bannedUntil")] = static_cast<boost::int64_t>(info.banned_until);
	ret[sslit("createdTime")] = static_cast<boost::uint64_t>(info.created_time);
	ret[sslit("createdIp")] = std::move(info.created_ip);
	ret[sslit("gender")] = AccountMap::get_attribute(info.account_id, AccountMap::ATTR_GENDER);
	ret[sslit("country")] = AccountMap::get_attribute(info.account_id, AccountMap::ATTR_COUNTRY);
//	ret[sslit("phoneNumber")] = AccountMap::get_attribute(info.account_id, AccountMap::ATTR_PHONE_NUMBER);
	ret[sslit("bankAccountName")] = AccountMap::get_attribute(info.account_id, AccountMap::ATTR_BANK_ACCOUNT_NAME);
	ret[sslit("bankName")] = AccountMap::get_attribute(info.account_id, AccountMap::ATTR_BANK_NAME);
	ret[sslit("bankAccountNumber")] = AccountMap::get_attribute(info.account_id, AccountMap::ATTR_BANK_ACCOUNT_NUMBER);
	ret[sslit("bankSwiftCode")] = AccountMap::get_attribute(info.account_id, AccountMap::ATTR_BANK_SWIFT_CODE);
	ret[sslit("remarks")] = AccountMap::get_attribute(info.account_id, AccountMap::ATTR_REMARKS);
	ret[sslit("maxVisibleSubordDepth")] = boost::lexical_cast<std::string>(max_visible_subord_depth);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
