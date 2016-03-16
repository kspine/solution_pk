#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/shared_recharge_map.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("getSharedRechargeListAll", session, params){
	const auto &begin_str = params.get("begin");
	const auto &count_str = params.get("count");

	Poseidon::JsonObject ret;

	std::vector<SharedRechargeMap::SharedRechargeInfo> shared_recharge_info_all;
	SharedRechargeMap::get_all(shared_recharge_info_all);

	auto begin = shared_recharge_info_all.begin(), end = shared_recharge_info_all.end();
	if(!begin_str.empty()){
		const auto begin_num = boost::lexical_cast<std::size_t>(begin_str);
		if(begin_num < shared_recharge_info_all.size()){
			begin = shared_recharge_info_all.begin() + static_cast<std::ptrdiff_t>(begin_num);
		} else {
			begin = shared_recharge_info_all.end();
		}
	}
	if(!count_str.empty()){
		const auto count_num = boost::lexical_cast<std::size_t>(count_str);
		if(count_num < static_cast<std::size_t>(end - begin)){
			end = begin + static_cast<std::ptrdiff_t>(count_num);
		} else {
			end = shared_recharge_info_all.end();
		}
	}

	Poseidon::JsonArray array;
	for(auto it = shared_recharge_info_all.begin(); it != shared_recharge_info_all.end(); ++it){
		Poseidon::JsonObject object;
		object[sslit("state")] = static_cast<unsigned>(it->state);
		object[sslit("amount")] = it->amount;
		const auto account_id = it->account_id;
		auto account_info = AccountMap::require(account_id);
		object[sslit("loginName")] = std::move(account_info.login_name);
		object[sslit("nick")] = std::move(account_info.nick);
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
	ret[sslit("sharedRechargeList")] = std::move(array);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
