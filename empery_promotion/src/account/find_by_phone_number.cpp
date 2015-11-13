#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("findByPhoneNumber", session, params){
	const auto phone_number = params.at("phoneNumber");

	Poseidon::JsonObject ret;
	std::vector<AccountMap::AccountInfo> infos;
	AccountMap::get_by_phone_number(infos, phone_number);
	if(infos.empty()){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	Poseidon::JsonArray accounts;
	for(auto it = infos.begin(); it != infos.end(); ++it){
		auto &info = *it;
		Poseidon::JsonObject account;
		account[sslit("loginName")] = std::move(info.login_name);
		account[sslit("phoneNumber")] = std::move(info.phone_number);
		account[sslit("nick")] = std::move(info.nick);
		account[sslit("flags")] = info.flags;
		account[sslit("createdTime")] = static_cast<boost::uint64_t>(info.created_time);
		account[sslit("createdIp")] = std::move(info.created_ip);
		accounts.emplace_back(std::move(account));
	}
	ret[sslit("accounts")] = std::move(accounts);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
