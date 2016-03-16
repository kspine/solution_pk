#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/shared_recharge_map.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("getSharedRechargeList", session, params){
	const auto &login_name = params.get("loginName");

	Poseidon::JsonObject ret;

	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	const auto account_id = info.account_id;

	std::vector<SharedRechargeMap::SharedRechargeInfo> shared_recharge_info_all;
	SharedRechargeMap::get_by_account(shared_recharge_info_all, account_id);

	Poseidon::JsonArray array;
	for(auto it = shared_recharge_info_all.begin(); it != shared_recharge_info_all.end(); ++it){
		Poseidon::JsonObject object;
		object[sslit("state")] = static_cast<unsigned>(it->state);
		object[sslit("amount")] = it->amount;
		const auto recharge_to_account_id = it->recharge_to_account_id;
		auto recharge_to_account_info = AccountMap::require(recharge_to_account_id);
		object[sslit("rechargeToLoginName")] = std::move(recharge_to_account_info.login_name);
		object[sslit("rechargeToNick")] = std::move(recharge_to_account_info.nick);
		object[sslit("bankAccountName")] = AccountMap::get_attribute(recharge_to_account_id, AccountMap::ATTR_BANK_ACCOUNT_NAME);
		object[sslit("bankName")] = AccountMap::get_attribute(recharge_to_account_id, AccountMap::ATTR_BANK_NAME);
		object[sslit("bankAccountNumber")] = AccountMap::get_attribute(recharge_to_account_id, AccountMap::ATTR_BANK_ACCOUNT_NUMBER);
		object[sslit("bankSwiftCode")] = AccountMap::get_attribute(recharge_to_account_id, AccountMap::ATTR_BANK_SWIFT_CODE);
		array.emplace_back(std::move(object));
	}
	ret[sslit("sharedRechargeListRequested")] = std::move(array);

	shared_recharge_info_all.clear();
	SharedRechargeMap::get_by_recharge_to_account(shared_recharge_info_all, account_id);

	array.clear();
	for(auto it = shared_recharge_info_all.begin(); it != shared_recharge_info_all.end(); ++it){
		Poseidon::JsonObject object;
		object[sslit("state")] = static_cast<unsigned>(it->state);
		object[sslit("amount")] = it->amount;
		const auto account_id = it->recharge_to_account_id;
		auto account_info = AccountMap::require(account_id);
		object[sslit("loginName")] = std::move(account_info.login_name);
		object[sslit("nick")] = std::move(account_info.nick);
		array.emplace_back(std::move(object));
	}
	ret[sslit("sharedRechargeListAccepted")] = std::move(array);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
