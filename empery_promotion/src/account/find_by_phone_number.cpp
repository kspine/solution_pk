#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("findByPhoneNumber", /* session */, params){
	const auto phoneNumber = params.at("phoneNumber");

	Poseidon::JsonObject ret;
	std::vector<AccountMap::AccountInfo> infos;
	AccountMap::getByPhoneNumber(infos, phoneNumber);
	if(infos.empty()){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	Poseidon::JsonArray accounts;
	for(auto it = infos.begin(); it != infos.end(); ++it){
		auto &info = *it;
		Poseidon::JsonObject account;
		account[sslit("loginName")] = std::move(info.loginName);
		account[sslit("phoneNumber")] = std::move(info.phoneNumber);
		account[sslit("nick")] = std::move(info.nick);
		account[sslit("flags")] = info.flags;
		account[sslit("createdTime")] = static_cast<boost::uint64_t>(info.createdTime);
		account[sslit("createdIp")] = std::move(info.createdIp);
		accounts.emplace_back(std::move(account));
	}
	ret[sslit("accounts")] = std::move(accounts);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
