#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("getPyramid", /* session */, params){
	const auto &loginName = params.at("loginName");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	Poseidon::JsonArray members;
	std::deque<std::pair<AccountId, Poseidon::JsonArray *>> queue;
	queue.emplace_back(info.accountId, &members);
	std::vector<AccountMap::AccountInfo> memberInfos;
	for(unsigned depth = 0; (depth < 32) && !queue.empty(); ++depth){
		const auto currentAccountId = queue.front().first;
		auto &currentMembers = *(queue.front().second);

		memberInfos.clear();
		AccountMap::getByReferrerId(memberInfos, currentAccountId);
		for(auto it = memberInfos.begin(); it != memberInfos.end(); ++it){
			currentMembers.emplace_back(Poseidon::JsonObject());
			auto &member = currentMembers.back().get<Poseidon::JsonObject>();

			const auto level = AccountMap::getAttribute(it->accountId, AccountMap::ATTR_ACCOUNT_LEVEL);

			member[sslit("loginName")] = std::move(it->loginName);
			member[sslit("nick")] = std::move(it->nick);
			member[sslit("level")] = boost::lexical_cast<std::string>(level);
			member[sslit("members")] = Poseidon::JsonArray();

			queue.emplace_back(it->accountId, &(member.at(sslit("members")).get<Poseidon::JsonArray>()));
		}

		queue.pop_front();
	}
	ret[sslit("members")] = std::move(members);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
