#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../item_ids.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("queryAccountPerformance", session, params){
	const auto &login_name = params.at("loginName");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	boost::container::flat_map<ItemId, boost::uint64_t> performance_map, current_map;

	std::deque<AccountMap::AccountInfo> queue;
	std::vector<AccountMap::AccountInfo> temp_accounts;
	AccountMap::get_by_referrer_id(temp_accounts, info.account_id);
	std::copy(std::make_move_iterator(temp_accounts.begin()), std::make_move_iterator(temp_accounts.end()),
		std::back_inserter(queue));
	while(!queue.empty()){
		const auto cur_account_id = queue.front().account_id;
		queue.pop_front();

		temp_accounts.clear();
		AccountMap::get_by_referrer_id(temp_accounts, cur_account_id);
		std::copy(std::make_move_iterator(temp_accounts.begin()), std::make_move_iterator(temp_accounts.end()),
			std::back_inserter(queue));

		current_map.clear();
		ItemMap::get_all_by_account_id(current_map, cur_account_id);
		for(auto it = current_map.begin(); it != current_map.end(); ++it){
			auto &count = performance_map[it->first];
			count = checked_add(count, it->second);
		}
	}

	Poseidon::JsonObject items;
	for(auto it = performance_map.begin(); it != performance_map.end(); ++it){
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
