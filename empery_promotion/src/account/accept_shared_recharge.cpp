#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/shared_recharge_map.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/item_map.hpp"
#include "../item_ids.hpp"
#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("acceptSharedRecharge", session, params){
	const auto &login_name = params.get("loginName");
	const auto &recharge_to_login_name = params.get("rechargeToLoginName");

	Poseidon::JsonObject ret;

	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	const auto account_id = info.account_id;

	info = AccountMap::get_by_login_name(recharge_to_login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	const auto recharge_to_account_id = info.account_id;

	const auto recharge_info = SharedRechargeMap::get(account_id, recharge_to_account_id);
	if(recharge_info.state != SharedRechargeMap::ST_REQUESTING){
		ret[sslit("errorCode")] = (int)Msg::ERR_SHARED_RECHARGE_NOT_REQUESTING;
		ret[sslit("errorMessage")] = "No requesting shared recharge found";
		return ret;
	}
	const auto balance_amount = ItemMap::get_count(recharge_to_account_id, ItemIds::ID_ACCOUNT_BALANCE);
	if(balance_amount < recharge_info.amount){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ITEMS;
		ret[sslit("errorMessage")] = "No enough account balance";
		return ret;
	}

	SharedRechargeMap::accept(account_id, recharge_to_account_id);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
