#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/wd_slip.hpp"
#include "../bill_states.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("getWithdrawalRequests", /* session */, params){
	const auto &loginName = params.get("loginName");
	const auto &begin = params.get("begin");
	const auto &count = params.get("count");
	const auto &timeBegin = params.get("timeBegin");
	const auto &timeEnd = params.get("timeEnd");

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

	std::vector<boost::shared_ptr<MySql::Promotion_WdSlip> > objs;
	std::ostringstream oss;
	oss <<"SELECT * FROM `Promotion_WdSlip` WHERE 1=1 ";
	if(!timeBegin.empty()){
		char str[256];
		Poseidon::formatTime(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(timeBegin), false);
		oss <<"AND '" <<str <<"' <= `timestamp` ";
		Poseidon::formatTime(str, sizeof(str), boost::lexical_cast<boost::uint64_t>(timeEnd), false);
		oss <<"AND `timestamp` < '" <<str <<"' ";
	}
	if(!loginName.empty()){
		oss <<"AND `accountId` = " <<accountId <<" ";
	}
	oss <<"AND `state` < " <<(unsigned)BillStates::ST_CANCELLED;
	if(!begin.empty()){
		auto numBegin = boost::lexical_cast<boost::uint64_t>(begin);
		auto numCount = boost::lexical_cast<boost::uint64_t>(count);
		oss <<"LIMIT " <<numBegin <<", " <<numCount;
	}
	MySql::Promotion_WdSlip::batchLoad(objs, oss.str());

	Poseidon::JsonArray requests;
	for(auto it = objs.begin(); it != objs.end(); ++it){
		const auto &obj = *it;
		Poseidon::JsonObject elem;
		elem[sslit("serial")] = obj->get_serial();
		elem[sslit("createdTime")] = obj->get_createdTime();
		elem[sslit("amount")] = obj->get_amount();
		elem[sslit("fee")] = obj->get_fee();
		elem[sslit("remarks")] = obj->unlockedGet_remarks();
		requests.emplace_back(std::move(elem));
	}
	ret[sslit("requests")] = std::move(requests);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
