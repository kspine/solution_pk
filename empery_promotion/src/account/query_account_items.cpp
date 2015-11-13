#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../item_ids.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("queryAccountItems", session, params){
	const auto &login_name = params.at("loginName");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	Poseidon::JsonObject items;
	boost::container::flat_map<ItemId, boost::uint64_t> item_map;
	ItemMap::get_all_by_account_id(item_map, info.account_id);
	for(auto it = item_map.begin(); it != item_map.end(); ++it){
		char str[256];
		unsigned len = (unsigned)std::sprintf(str, "%llu", (unsigned long long)it->first.get());
		items[SharedNts(str, len)] = it->second;
	}

	ret[sslit("nick")] = std::move(info.nick);
	ret[sslit("phoneNumber")] = std::move(info.phone_number);
	ret[sslit("level")] = boost::lexical_cast<std::string>(info.level);
	ret[sslit("items")] = std::move(items);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
