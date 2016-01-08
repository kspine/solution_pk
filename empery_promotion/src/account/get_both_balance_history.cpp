#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {
	namespace {

#define MYSQL_OBJECT_NAME	BothBalanceHistoryResult
#define MYSQL_OBJECT_FIELDS	\
	FIELD_BIGINT_UNSIGNED	(account_id)	\
	FIELD_DATETIME			(timestamp)	\
	FIELD_BIGINT			(delta_balance)	\
	FIELD_INTEGER_UNSIGNED	(reason)	\
	FIELD_BIGINT_UNSIGNED	(param1)	\
	FIELD_BIGINT_UNSIGNED	(param2)	\
	FIELD_BIGINT_UNSIGNED	(param3)	\
	FIELD_STRING			(remarks)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME	BothSumRows
#define MYSQL_OBJECT_FIELDS	\
	FIELD_BIGINT			(sum)	\
	FIELD_BIGINT_UNSIGNED	(rows)
#include <poseidon/mysql/object_generator.hpp>

	}
}

ACCOUNT_SERVLET("getBothBalanceHistory", session, params){
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

	std::vector<boost::shared_ptr<MySql::BothBalanceHistoryResult>> objs;
	std::ostringstream oss_in, oss_out;
	oss_in  <<"SELECT ";
	oss_out <<"SELECT ";
	if(brief_mode.empty()){
		oss_in  <<"*,   CAST(`income_balance`  AS SIGNED) AS `delta_balance` ";
		oss_out <<"*, - CAST(`outcome_balance` AS SIGNED) AS `delta_balance` ";
	} else {
		oss_in  <<"SUM(  CAST(`income_balance`  AS SIGNED)) AS `sum`, COUNT(*) AS `rows` ";
		oss_out <<"SUM(- CAST(`outcome_balance` AS SIGNED)) AS `sum`, COUNT(*) AS `rows` ";
	}
	oss_in  <<"FROM `Promotion_IncomeBalanceHistory`  WHERE 1=1 ";
	oss_out <<"FROM `Promotion_OutcomeBalanceHistory` WHERE 1=1 ";
	if(!time_begin.empty()){
		char str[256];
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<std::uint64_t>(time_begin), false);
		oss_in  <<"AND " <<Poseidon::MySql::StringEscaper(str) <<" <= `timestamp` ";
		oss_out <<"AND " <<Poseidon::MySql::StringEscaper(str) <<" <= `timestamp` ";
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<std::uint64_t>(time_end), false);
		oss_in  <<"AND `timestamp` < " <<Poseidon::MySql::StringEscaper(str) <<" ";
		oss_out <<"AND `timestamp` < " <<Poseidon::MySql::StringEscaper(str) <<" ";
	}
	if(!reason.empty()){
		auto enum_reason = boost::lexical_cast<std::uint32_t>(reason);
		oss_in  <<"AND `reason` = " <<enum_reason <<" ";
		oss_out <<"AND `reason` = " <<enum_reason <<" ";
	}
	if(!login_name.empty()){
		oss_in  <<"AND `account_id` = " <<account_id <<" ";
		oss_out <<"AND `account_id` = " <<account_id <<" ";
	}
	std::ostringstream oss;
	oss <<"(" <<oss_in.str() <<") UNION ALL (" <<oss_out.str() <<") ";
	if(brief_mode.empty()){
		oss <<"ORDER BY `timestamp` DESC, `auto_id` DESC ";
		if(!count.empty()){
			oss <<"LIMIT ";
			if(!begin.empty()){
				auto num_begin = boost::lexical_cast<std::uint64_t>(begin);
				oss <<num_begin <<", ";
			}
			auto num_count = boost::lexical_cast<std::uint64_t>(count);
			oss <<num_count;
		}
		MySql::BothBalanceHistoryResult::batch_load(objs, oss.str());

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
			elem[sslit("deltaBalance")] = obj->get_delta_balance();
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
		std::vector<boost::shared_ptr<MySql::BothSumRows>> results;
		MySql::BothSumRows::batch_load(results, oss.str());
		std::int64_t sum = 0;
		std::uint64_t rows = 0;
		for(auto it = results.begin(); it != results.end(); ++it){
			const auto &obj = *it;
			sum  += obj->get_sum();
			rows += obj->get_rows();
		}
		ret[sslit("sum")] = sum;
		ret[sslit("rows")] = rows;
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
