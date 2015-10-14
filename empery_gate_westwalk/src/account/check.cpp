#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"

namespace TexasGateWestwalk {

ACCOUNT_SERVLET("check", /* session */, params){
	const AUTO_REF(accountName, params.at("accountName"));

	Poseidon::JsonObject ret;
	if(AccountMap::has(accountName)){
		ret[sslit("errCode")] = (int)Msg::ERR_DUPLICATE_ACCOUNT;
		ret[sslit("msg")] = "Another account with the same name already exists";
		return ret;
	}

	ret[sslit("errCode")] = (int)Msg::ST_OK;
	ret[sslit("msg")] = "No error";
	return ret;
}

}
