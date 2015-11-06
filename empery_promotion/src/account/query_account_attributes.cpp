#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("queryAccountAttributes", session, params){
	const auto &loginName = params.at("loginName");

	Poseidon::JsonObject ret;
	auto info = AccountMap::getByLoginName(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	std::string referrerLoginName, referrerNick;
	if(info.referrerId){
		auto referrerInfo = AccountMap::require(info.referrerId);
		referrerLoginName = std::move(referrerInfo.loginName);
		referrerNick = std::move(referrerInfo.nick);
	}

	boost::uint64_t maxVisibleSubordDepth;
	const auto &str = AccountMap::getAttribute(info.accountId, AccountMap::ATTR_MAX_VISIBLE_SUBORD_DEPTH);
	if(str.empty()){
		maxVisibleSubordDepth = getConfig<boost::uint64_t>("default_max_visible_subord_depth", 2);
	} else {
		maxVisibleSubordDepth = boost::lexical_cast<boost::uint64_t>(str);
	}

	ret[sslit("phoneNumber")] = std::move(info.phoneNumber);
	ret[sslit("nick")] = std::move(info.nick);
	ret[sslit("level")] = boost::lexical_cast<std::string>(info.level);
	ret[sslit("referrerLoginName")] = std::move(referrerLoginName);
	ret[sslit("referrerNick")] = std::move(referrerNick);
	ret[sslit("flags")] = info.flags;
	ret[sslit("bannedUntil")] = static_cast<boost::int64_t>(info.bannedUntil);
	ret[sslit("createdTime")] = static_cast<boost::uint64_t>(info.createdTime);
	ret[sslit("createdIp")] = std::move(info.createdIp);
	ret[sslit("gender")] = AccountMap::getAttribute(info.accountId, AccountMap::ATTR_GENDER);
	ret[sslit("country")] = AccountMap::getAttribute(info.accountId, AccountMap::ATTR_COUNTRY);
//	ret[sslit("phoneNumber")] = AccountMap::getAttribute(info.accountId, AccountMap::ATTR_PHONE_NUMBER);
	ret[sslit("bankAccountName")] = AccountMap::getAttribute(info.accountId, AccountMap::ATTR_BANK_ACCOUNT_NAME);
	ret[sslit("bankName")] = AccountMap::getAttribute(info.accountId, AccountMap::ATTR_BANK_NAME);
	ret[sslit("bankAccountNumber")] = AccountMap::getAttribute(info.accountId, AccountMap::ATTR_BANK_ACCOUNT_NUMBER);
	ret[sslit("bankSwiftCode")] = AccountMap::getAttribute(info.accountId, AccountMap::ATTR_BANK_SWIFT_CODE);
	ret[sslit("remarks")] = AccountMap::getAttribute(info.accountId, AccountMap::ATTR_REMARKS);
	ret[sslit("maxVisibleSubordDepth")] = boost::lexical_cast<std::string>(maxVisibleSubordDepth);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
