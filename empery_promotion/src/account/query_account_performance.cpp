#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../item_ids.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("queryAccountPerformance", session, params){
	const auto &loginName = params.at("loginName");

	Poseidon::JsonObject ret;
	auto info = AccountMap::getByLoginName(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	boost::container::flat_map<ItemId, boost::uint64_t> performanceMap, currentMap;

	std::deque<AccountMap::AccountInfo> queue;
	std::vector<AccountMap::AccountInfo> tempAccounts;
	AccountMap::getByReferrerId(tempAccounts, info.accountId);
	std::copy(std::make_move_iterator(tempAccounts.begin()), std::make_move_iterator(tempAccounts.end()),
		std::back_inserter(queue));
	while(!queue.empty()){
		const auto curAccountId = queue.front().accountId;
		queue.pop_front();

		tempAccounts.clear();
		AccountMap::getByReferrerId(tempAccounts, curAccountId);
		std::copy(std::make_move_iterator(tempAccounts.begin()), std::make_move_iterator(tempAccounts.end()),
			std::back_inserter(queue));

		currentMap.clear();
		ItemMap::getAllByAccountId(currentMap, curAccountId);
		for(auto it = currentMap.begin(); it != currentMap.end(); ++it){
			auto &count = performanceMap[it->first];
			count = checkedAdd(count, it->second);
		}
	}

	Poseidon::JsonObject items;
	for(auto it = performanceMap.begin(); it != performanceMap.end(); ++it){
		char str[256];
		unsigned len = (unsigned)std::sprintf(str, "%llu", (unsigned long long)it->first.get());
		items[SharedNts(str, len)] = it->second;
	}

	ret[sslit("nick")] = std::move(info.nick);
	ret[sslit("phoneNumber")] = std::move(info.phoneNumber);
	ret[sslit("level")] = boost::lexical_cast<std::string>(info.level);
	ret[sslit("items")] = std::move(items);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
