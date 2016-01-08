#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/account.hpp"
#include "../mysql/sum_rows.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("getAccountList", session, params){
	const auto &fetch_all_data = params.get("fetchAllData");
	const auto &login_name = params.get("loginName");
	const auto &begin = params.get("begin");
	const auto &count = params.get("count");
	const auto &time_begin = params.get("timeBegin");
	const auto &time_end = params.get("timeEnd");
	const auto &brief_mode = params.get("briefMode");

	Poseidon::JsonObject ret;

	AccountId account_id;
	if(fetch_all_data.empty() || !login_name.empty()){
		auto info = AccountMap::get_by_login_name(login_name);
		if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
			ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
			ret[sslit("errorMessage")] = "Account is not found";
			return ret;
		}
		account_id = info.account_id;
	}

	std::vector<boost::shared_ptr<MySql::Promotion_Account>> objs;
	std::ostringstream oss;
	oss <<"SELECT ";
	if(brief_mode.empty()){
		oss <<"* ";
	} else {
		oss <<"0 AS `sum`, COUNT(*) AS `rows` ";
	}
	oss <<"FROM `Promotion_Account` WHERE 1=1 ";
	if(!time_begin.empty()){
		char str[256];
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<std::uint64_t>(time_begin), false);
		oss <<"AND " <<Poseidon::MySql::StringEscaper(str) <<" <= `created_time` ";
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<std::uint64_t>(time_end), false);
		oss <<"AND `created_time` < " <<Poseidon::MySql::StringEscaper(str) <<" ";
	}
	if(!login_name.empty()){
		oss <<"AND `account_id` = " <<account_id <<" ";
	}
	oss <<"ORDER BY `created_time` DESC ";
	if(brief_mode.empty()){
		if(!count.empty()){
			oss <<"LIMIT ";
			if(!begin.empty()){
				auto num_begin = boost::lexical_cast<std::uint64_t>(begin);
				oss <<num_begin <<", ";
			}
			auto num_count = boost::lexical_cast<std::uint64_t>(count);
			oss <<num_count;
		}
		MySql::Promotion_Account::batch_load(objs, oss.str());

		Poseidon::JsonArray accounts;
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = *it;

			auto info = AccountMap::get(AccountId(obj->get_account_id()));
			if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
				LOG_EMPERY_PROMOTION_WARNING("No such account: account_id = ", info.account_id);
				continue;
			}
			auto referrer_info = AccountMap::get(info.referrer_id);

			Poseidon::JsonObject items;
			boost::container::flat_map<ItemId, std::uint64_t> item_map;
			ItemMap::get_all_by_account_id(item_map, info.account_id);
			for(auto it = item_map.begin(); it != item_map.end(); ++it){
				char str[256];
				unsigned len = (unsigned)std::sprintf(str, "%llu", (unsigned long long)it->first.get());
				items[SharedNts(str, len)] = it->second;
			}

			Poseidon::JsonObject elem;
			elem[sslit("accountId")] = info.account_id.get();
			elem[sslit("loginName")] = std::move(info.login_name);
			elem[sslit("nick")] = std::move(info.nick);
			elem[sslit("level")] = boost::lexical_cast<std::string>(info.level);
			elem[sslit("referrerId")] = referrer_info.account_id.get();
			elem[sslit("referrerLoginName")] = std::move(referrer_info.login_name);
			elem[sslit("createdTime")] = info.created_time;
			elem[sslit("createdIp")] = std::move(info.created_ip);
			elem[sslit("items")] = std::move(items);

			accounts.emplace_back(std::move(elem));
		}
		ret[sslit("accounts")] = std::move(accounts);
	} else {
		const auto obj = boost::make_shared<MySql::Promotion_SumRows>();
		obj->load_and_wait(oss.str());
		// ret[sslit("sum")] = obj->get_sum();
		ret[sslit("rows")] = obj->get_rows();
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
