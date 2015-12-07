#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/outcome_balance_history.hpp"
#include "../mysql/sum_rows.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("getOutcomeBalanceHistory", session, params){
	const auto &fetch_all_data = params.get("fetchAllData");
	const auto &login_name = params.get("loginName");
	const auto &begin = params.get("begin");
	const auto &count = params.get("count");
	const auto &reason = params.get("reason");
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

	std::vector<boost::shared_ptr<MySql::Promotion_OutcomeBalanceHistory>> objs;
	std::ostringstream oss;
	oss <<"SELECT ";
	if(brief_mode.empty()){
		oss <<"* ";
	} else {
		oss <<"SUM(`outcome_balance`) AS `sum`, COUNT(*) AS `rows` ";
	}
	oss <<"FROM `Promotion_OutcomeBalanceHistory` WHERE 1=1 ";
	if(!time_begin.empty()){
		char str[256];
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(time_begin), false);
		oss <<"AND '" <<str <<"' <= `timestamp` ";
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(time_end), false);
		oss <<"AND `timestamp` < '" <<str <<"' ";
	}
	if(!reason.empty()){
		auto enum_reason = boost::lexical_cast<boost::uint32_t>(reason);
		oss <<"AND `reason` = " <<enum_reason <<" ";
	}
	if(!login_name.empty()){
		oss <<"AND `account_id` = " <<account_id <<" ";
	}
	if(brief_mode.empty()){
		oss <<"ORDER BY `timestamp` DESC, `auto_id` DESC ";
		if(!count.empty()){
			oss <<"LIMIT ";
			if(!begin.empty()){
				auto num_begin = boost::lexical_cast<boost::uint64_t>(begin);
				oss <<num_begin <<", ";
			}
			auto num_count = boost::lexical_cast<boost::uint64_t>(count);
			oss <<num_count;
		}
		MySql::Promotion_OutcomeBalanceHistory::batch_load(objs, oss.str());

		Poseidon::JsonArray history;
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = *it;

			auto info = AccountMap::get(AccountId(obj->get_account_id()));
			if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
				LOG_EMPERY_PROMOTION_WARNING("No such account: account_id = ", info.account_id);
				continue;
			}

			Poseidon::JsonObject elem;
			elem[sslit("timestamp")] = obj->get_timestamp();
			elem[sslit("outcomeBalance")] = obj->get_outcome_balance();
			elem[sslit("reason")] = obj->get_reason();
			elem[sslit("param1")] = obj->get_param1();
			elem[sslit("param2")] = obj->get_param2();
			elem[sslit("param3")] = obj->get_param3();
			elem[sslit("remarks")] = obj->unlocked_get_remarks();
			elem[sslit("loginName")] = std::move(info.login_name);
			elem[sslit("nick")] = std::move(info.nick);
			history.emplace_back(std::move(elem));
		}
		ret[sslit("history")] = std::move(history);
	} else {
		const auto obj = boost::make_shared<MySql::Promotion_SumRows>();
		obj->load_and_wait(oss.str());
		ret[sslit("sum")] = obj->get_sum();
		ret[sslit("rows")] = obj->get_rows();
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
