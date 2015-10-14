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

	Poseidon::JsonObject items;
	boost::container::flat_map<ItemId, boost::uint64_t> itemMap;
	ItemMap::getAllByAccountId(itemMap, info.accountId);
	for(auto it = itemMap.begin(); it != itemMap.end(); ++it){
		char str[256];
		unsigned len = (unsigned)std::sprintf(str, "%llu", (unsigned long long)it->first.get());
		items[SharedNts(str, len)] = it->second;
	}

	const auto level = AccountMap::castAttribute<boost::uint64_t>(info.accountId, AccountMap::ATTR_ACCOUNT_LEVEL);

	ret[sslit("nick")] = std::move(info.nick);
	ret[sslit("phoneNumber")] = std::move(info.phoneNumber);
	ret[sslit("level")] = boost::lexical_cast<std::string>(level);
	ret[sslit("items")] = std::move(items);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
