#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/outcome_balance_history.hpp"
#include "../mysql/sum_rows.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("getOutcomeBalanceHistory", /* session */, params){
	const auto &loginName = params.get("loginName");
	const auto &begin = params.get("begin");
	const auto &count = params.get("count");
	const auto &reason = params.get("reason");
	const auto &timeBegin = params.get("timeBegin");
	const auto &timeEnd = params.get("timeEnd");
	const auto &briefMode = params.get("briefMode");

	Poseidon::JsonObject ret;

	AccountId accountId;
	if(!loginName.empty()){
		auto info = AccountMap::get(loginName);
		if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
			ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
			ret[sslit("errorMessage")] = "Account is not found";
			return ret;
		}
		accountId = info.accountId;
	}

	std::vector<boost::shared_ptr<MySql::Promotion_OutcomeBalanceHistory>> objs;
	std::ostringstream oss;
	oss <<"SELECT ";
	if(briefMode.empty()){
		oss <<"* ";
	} else {
		oss <<"SUM(`outcomeBalance`) AS `sum`, COUNT(*) AS `rows` ";
	}
	oss <<"FROM `Promotion_OutcomeBalanceHistory` WHERE 1=1 ";
	if(!timeBegin.empty()){
		char str[256];
		Poseidon::formatTime(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(timeBegin), false);
		oss <<"AND '" <<str <<"' <= `timestamp` ";
		Poseidon::formatTime(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(timeEnd), false);
		oss <<"AND `timestamp` < '" <<str <<"' ";
	}
	if(!reason.empty()){
		auto enumReason = boost::lexical_cast<boost::uint32_t>(reason);
		oss <<"AND `reason` = " <<enumReason <<" ";
	}
	if(!loginName.empty()){
		oss <<"AND `accountId` = " <<accountId <<" ";
	}
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
		MySql::Promotion_OutcomeBalanceHistory::batchLoad(objs, oss.str());

		Poseidon::JsonArray history;
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = *it;
			Poseidon::JsonObject elem;
			elem[sslit("timestamp")] = obj->get_timestamp();
			elem[sslit("outcomeBalance")] = obj->get_outcomeBalance();
			elem[sslit("reason")] = obj->get_reason();
			elem[sslit("param1")] = obj->get_param1();
			elem[sslit("param2")] = obj->get_param2();
			elem[sslit("param3")] = obj->get_param3();
			elem[sslit("remarks")] = obj->unlockedGet_remarks();
			history.emplace_back(std::move(elem));
		}
		ret[sslit("history")] = std::move(history);
	} else {
		const auto obj = boost::make_shared<MySql::Promotion_SumRows>();
		obj->syncLoad(oss.str());
		ret[sslit("sum")] = obj->get_sum();
		ret[sslit("rows")] = obj->get_rows();
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
