#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/account.hpp"
#include "../mysql/sum_rows.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("getAccountList", session, params){
	const auto &fetchAllData = params.get("fetchAllData");
	const auto &loginName = params.get("loginName");
	const auto &begin = params.get("begin");
	const auto &count = params.get("count");
	const auto &timeBegin = params.get("timeBegin");
	const auto &timeEnd = params.get("timeEnd");
	const auto &briefMode = params.get("briefMode");

	Poseidon::JsonObject ret;

	AccountId accountId;
	if(fetchAllData.empty() || !loginName.empty()){
		auto info = AccountMap::getByLoginName(loginName);
		if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
			ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
			ret[sslit("errorMessage")] = "Account is not found";
			return ret;
		}
		accountId = info.accountId;
	}

	std::vector<boost::shared_ptr<MySql::Promotion_Account>> objs;
	std::ostringstream oss;
	oss <<"SELECT ";
	if(briefMode.empty()){
		oss <<"* ";
	} else {
		oss <<"0 AS `sum`, COUNT(*) AS `rows` ";
	}
	oss <<"FROM `Promotion_Account` WHERE 1=1 ";
	if(!timeBegin.empty()){
		char str[256];
		Poseidon::formatTime(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(timeBegin), false);
		oss <<"AND '" <<str <<"' <= `createdTime` ";
		Poseidon::formatTime(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(timeEnd), false);
		oss <<"AND `createdTime` < '" <<str <<"' ";
	}
	if(!loginName.empty()){
		oss <<"AND `accountId` = " <<accountId <<" ";
	}
	oss <<"ORDER BY `createdTime` DESC ";
	if(briefMode.empty()){
		if(!count.empty()){
			oss <<"LIMIT ";
			if(!begin.empty()){
				auto numBegin = boost::lexical_cast<boost::uint64_t>(begin);
				oss <<numBegin <<", ";
			}
			auto numCount = boost::lexical_cast<boost::uint64_t>(count);
			oss <<numCount;
		}
		MySql::Promotion_Account::batchLoad(objs, oss.str());

		Poseidon::JsonArray accounts;
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = *it;

			auto info = AccountMap::get(AccountId(obj->get_accountId()));
			if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
				LOG_EMPERY_PROMOTION_WARNING("No such account: accountId = ", info.accountId);
				continue;
			}
			auto referrerInfo = AccountMap::get(info.referrerId);

			Poseidon::JsonObject items;
			boost::container::flat_map<ItemId, boost::uint64_t> itemMap;
			ItemMap::getAllByAccountId(itemMap, info.accountId);
			for(auto it = itemMap.begin(); it != itemMap.end(); ++it){
				char str[256];
				unsigned len = (unsigned)std::sprintf(str, "%llu", (unsigned long long)it->first.get());
				items[SharedNts(str, len)] = it->second;
			}

			Poseidon::JsonObject elem;
			elem[sslit("accountId")] = info.accountId.get();
			elem[sslit("loginName")] = std::move(info.loginName);
			elem[sslit("nick")] = std::move(info.nick);
			elem[sslit("level")] = boost::lexical_cast<std::string>(info.level);
			elem[sslit("referrerId")] = referrerInfo.accountId.get();
			elem[sslit("referrerLoginName")] = std::move(referrerInfo.loginName);
			elem[sslit("createdTime")] = info.createdTime;
			elem[sslit("createdIp")] = std::move(info.createdIp);
			elem[sslit("items")] = std::move(items);

			accounts.emplace_back(std::move(elem));
		}
		ret[sslit("accounts")] = std::move(accounts);
	} else {
		const auto obj = boost::make_shared<MySql::Promotion_SumRows>();
		obj->syncLoad(oss.str());
		// ret[sslit("sum")] = obj->get_sum();
		ret[sslit("rows")] = obj->get_rows();
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
