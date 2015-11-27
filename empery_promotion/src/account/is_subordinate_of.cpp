#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../events/account.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("isSubordinateOf", session, params){
	const auto &login_name = params.at("loginName");
	const auto &referrer_login_name = params.at("referrerLoginName");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	auto referrer_info = AccountMap::get_by_login_name(referrer_login_name);
	if(Poseidon::has_none_flags_of(referrer_info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_REFERRER;
		ret[sslit("errorMessage")] = "Referrer is not found";
		return ret;
	}

	auto current_referrer_id = info.referrer_id;
	boost::uint64_t depth = 1;
	for(;;){
		if(!current_referrer_id){
			ret[sslit("errorCode")] = (int)Msg::ERR_NOT_SUBORDINATE;
			ret[sslit("errorMessage")] = "Subordinate is not found";
			return ret;
		}
		if(current_referrer_id == referrer_info.account_id){
			break;
		}
		current_referrer_id = AccountMap::require(current_referrer_id).referrer_id;
		++depth;
	}

	const auto referrer_id = referrer_info.account_id;

	boost::uint64_t max_visible_subord_depth;
	const auto &str = AccountMap::get_attribute(referrer_id, AccountMap::ATTR_MAX_VISIBLE_SUBORD_DEPTH);
	if(str.empty()){
		max_visible_subord_depth = get_config<boost::uint64_t>("default_max_visible_subord_depth", 2);
	} else {
		max_visible_subord_depth = boost::lexical_cast<boost::uint64_t>(str);
	}

	const bool can_view_account_performance = AccountMap::cast_attribute<bool>(referrer_id, AccountMap::ATTR_CAN_VIEW_ACCOUNT_PERFORMANCE);

	ret[sslit("depth")] = depth;
	ret[sslit("maxVisibleSubordDepth")] = max_visible_subord_depth;
	ret[sslit("canViewAccountPerformance")] = can_view_account_performance;

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
