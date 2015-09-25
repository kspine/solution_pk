#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../item_ids.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("queryAccountItems", /* session */, params){
	const auto &loginName = params.at("loginName");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	ret[sslit("nick")] = std::move(info.nick);
	ret[sslit("level")] = AccountMap::getAttribute(info.accountId, AccountMap::ATTR_ACCOUNT_LEVEL);
	ret[sslit("accelerationCards")] = ItemMap::getCount(info.accountId, ItemIds::ID_ACCELERATION_CARDS);
	ret[sslit("accountBalance")] = ItemMap::getCount(info.accountId, ItemIds::ID_ACCOUNT_BALANCE);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
