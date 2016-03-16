#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/shared_recharge_map.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("requestSharedRecharge", session, params){
	const auto &login_name = params.get("loginName");
	const auto amount = boost::lexical_cast<std::uint64_t>(params.get("amount"));

	Poseidon::JsonObject ret;

	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	const auto account_id = info.account_id;

	std::vector<SharedRechargeMap::SharedRechargeInfo> old_info_all;
	SharedRechargeMap::get_by_account(old_info_all, account_id);
	if(!old_info_all.empty()){
		ret[sslit("errorCode")] = (int)Msg::ERR_SHARED_RECHARGE_IN_PROGRESS;
		ret[sslit("errorMessage")] = "Another shared recharge is in progress";
		return ret;
	}

	std::vector<SharedRechargeMap::SharedRechargeInfo> candidate_info_all;
	SharedRechargeMap::request(candidate_info_all, account_id, amount);

	Poseidon::JsonArray array;
	for(auto it = candidate_info_all.begin(); it != candidate_info_all.end(); ++it){
		const auto candidate_account_id = it->recharge_to_account_id;
		auto recharge_to_info = AccountMap::require(candidate_account_id);
		Poseidon::JsonObject object;
		object[sslit("rechargeToLoginName")] = std::move(recharge_to_info.login_name);
		object[sslit("rechargeToPhoneNumber")] = std::move(recharge_to_info.phone_number);
		array.emplace_back(std::move(object));
	}
	ret[sslit("candidates")] = std::move(array);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
