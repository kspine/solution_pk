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
	FIELD_BIGINT_UNSIGNED	(accountId)	\
	FIELD_DATETIME			(timestamp)	\
	FIELD_BIGINT			(deltaBalance)	\
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

ACCOUNT_SERVLET("getBothBalanceHistory", /* session */, params){
	const auto &fetchAllData = params.get("fetchAllData");
	const auto &loginName = params.get("loginName");
	const auto &begin = params.get("begin");
	const auto &count = params.get("count");
	const auto &reason = params.get("reason");
	const auto &timeBegin = params.get("timeBegin");
	const auto &timeEnd = params.get("timeEnd");
	const auto &briefMode = params.get("briefMode");

	Poseidon::JsonObject ret;

	AccountId accountId;
	if(fetchAllData.empty() || !loginName.empty()){
		auto info = AccountMap::get(loginName);
		if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
			ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
			ret[sslit("errorMessage")] = "Account is not found";
			return ret;
		}
		accountId = info.accountId;
	}

	std::vector<boost::shared_ptr<MySql::BothBalanceHistoryResult>> objs;
	std::ostringstream ossIn, ossOut;
	ossIn  <<"SELECT ";
	ossOut <<"SELECT ";
	if(briefMode.empty()){
		ossIn  <<"*,   CAST(`incomeBalance`  AS SIGNED) AS `deltaBalance` ";
		ossOut <<"*, - CAST(`outcomeBalance` AS SIGNED) AS `deltaBalance` ";
	} else {
		ossIn  <<"SUM(  CAST(`incomeBalance`  AS SIGNED)) AS `sum`, COUNT(*) AS `rows` ";
		ossOut <<"SUM(- CAST(`outcomeBalance` AS SIGNED)) AS `sum`, COUNT(*) AS `rows` ";
	}
	ossIn  <<"FROM `Promotion_IncomeBalanceHistory`  WHERE 1=1 ";
	ossOut <<"FROM `Promotion_OutcomeBalanceHistory` WHERE 1=1 ";
	if(!timeBegin.empty()){
		char str[256];
		Poseidon::formatTime(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(timeBegin), false);
		ossIn  <<"AND '" <<str <<"' <= `timestamp` ";
		ossOut <<"AND '" <<str <<"' <= `timestamp` ";
		Poseidon::formatTime(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(timeEnd), false);
		ossIn  <<"AND `timestamp` < '" <<str <<"' ";
		ossOut <<"AND `timestamp` < '" <<str <<"' ";
	}
	if(!reason.empty()){
		auto enumReason = boost::lexical_cast<boost::uint32_t>(reason);
		ossIn  <<"AND `reason` = " <<enumReason <<" ";
		ossOut <<"AND `reason` = " <<enumReason <<" ";
	}
	if(!loginName.empty()){
		ossIn  <<"AND `accountId` = " <<accountId <<" ";
		ossOut <<"AND `accountId` = " <<accountId <<" ";
	}
	std::ostringstream oss;
	oss <<"(" <<ossIn.str() <<") UNION ALL (" <<ossOut.str() <<") ";
	if(briefMode.empty()){
		oss <<"ORDER BY `timestamp` DESC ";
		if(!count.empty()){
			oss <<"LIMIT ";
			if(!begin.empty()){
				auto numBegin = boost::lexical_cast<boost::uint64_t>(begin);
				oss <<numBegin <<", ";
			}
			auto numCount = boost::lexical_cast<boost::uint64_t>(count);
			oss <<numCount;
		}
		MySql::BothBalanceHistoryResult::batchLoad(objs, oss.str());

		Poseidon::JsonArray history;
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = *it;
			auto info = AccountMap::get(AccountId(obj->get_accountId()));
			Poseidon::JsonObject elem;
			elem[sslit("timestamp")] = obj->get_timestamp();
			elem[sslit("deltaBalance")] = obj->get_deltaBalance();
			elem[sslit("reason")] = obj->get_reason();
			elem[sslit("param1")] = obj->get_param1();
			elem[sslit("param2")] = obj->get_param2();
			elem[sslit("param3")] = obj->get_param3();
			elem[sslit("remarks")] = obj->unlockedGet_remarks();
			elem[sslit("loginName")] = std::move(info.loginName);
			history.emplace_back(std::move(elem));
		}
		ret[sslit("history")] = std::move(history);
	} else {
		std::vector<boost::shared_ptr<MySql::BothSumRows>> results;
		MySql::BothSumRows::batchLoad(results, oss.str());
		boost::int64_t sum = 0;
		boost::uint64_t rows = 0;
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
