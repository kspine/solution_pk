#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../events/account.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("isSubordinateOf", session, params){
	const auto &loginName = params.at("loginName");
	const auto &referrerLoginName = params.at("referrerLoginName");

	Poseidon::JsonObject ret;
	auto info = AccountMap::getByLoginName(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	auto referrerInfo = AccountMap::getByLoginName(referrerLoginName);
	if(Poseidon::hasNoneFlagsOf(referrerInfo.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_REFERRER;
		ret[sslit("errorMessage")] = "Referrer is not found";
		return ret;
	}

	auto currentReferrerId = info.referrerId;
	boost::uint64_t depth = 1;
	for(;;){
		if(!currentReferrerId){
			ret[sslit("errorCode")] = (int)Msg::ERR_NOT_SUBORDINATE;
			ret[sslit("errorMessage")] = "Subordinate is not found";
			return ret;
		}
		if(currentReferrerId == referrerInfo.accountId){
			break;
		}
		currentReferrerId = AccountMap::require(currentReferrerId).referrerId;
		++depth;
	}

	boost::uint64_t maxVisibleSubordDepth;
	const auto &str = AccountMap::getAttribute(referrerInfo.accountId, AccountMap::ATTR_MAX_VISIBLE_SUBORD_DEPTH);
	if(str.empty()){
		maxVisibleSubordDepth = getConfig<boost::uint64_t>("default_max_visible_subord_depth", 2);
	} else {
		maxVisibleSubordDepth = boost::lexical_cast<boost::uint64_t>(str);
	}

	ret[sslit("depth")] = depth;
	ret[sslit("maxVisibleSubordDepth")] = maxVisibleSubordDepth;

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
