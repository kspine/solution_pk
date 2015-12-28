#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/wd_slip.hpp"
#include "../mysql/sum_rows.hpp"
#include "../bill_states.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("getWithdrawalRequests", session, params){
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

	std::vector<boost::shared_ptr<MySql::Promotion_WdSlip>> objs;
	std::ostringstream oss;
	oss <<"SELECT ";
	if(brief_mode.empty()){
		oss <<"* ";
	} else {
		oss <<"SUM(`amount`) AS `sum`, COUNT(*) AS `rows` ";
	}
	oss <<"FROM `Promotion_WdSlip` WHERE 1=1 ";
	if(!time_begin.empty()){
		char str[256];
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<std::uint64_t>(time_begin), false);
		oss <<"AND '" <<str <<"' <= `created_time` ";
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<std::uint64_t>(time_end), false);
		oss <<"AND `created_time` < '" <<str <<"' ";
	}
	if(!login_name.empty()){
		oss <<"AND `account_id` = " <<account_id <<" ";
	}
	if(brief_mode.empty()){
		oss <<"ORDER BY `state` ASC, `created_time` DESC ";
		if(!count.empty()){
			oss <<"LIMIT ";
			if(!begin.empty()){
				auto num_begin = boost::lexical_cast<std::uint64_t>(begin);
				oss <<num_begin <<", ";
			}
			auto num_count = boost::lexical_cast<std::uint64_t>(count);
			oss <<num_count;
		}
		MySql::Promotion_WdSlip::batch_load(objs, oss.str());

		Poseidon::JsonArray requests;
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = *it;
			const auto account_id = AccountId(obj->get_account_id());

			auto info = AccountMap::get(account_id);
			if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
				LOG_EMPERY_PROMOTION_WARNING("No such account: account_id = ", account_id);
				continue;
			}

			Poseidon::JsonObject elem;
			elem[sslit("serial")]            = obj->get_serial();
			elem[sslit("createdTime")]       = obj->get_created_time();
			elem[sslit("amount")]            = obj->get_amount();
			elem[sslit("fee")]               = obj->get_fee();
			elem[sslit("state")]             = obj->get_state();
			elem[sslit("remarks")]           = obj->unlocked_get_remarks();
			elem[sslit("loginName")]         = std::move(info.login_name);
			elem[sslit("nick")]              = std::move(info.nick);
			elem[sslit("bankAccountName")]   = AccountMap::get_attribute(account_id, AccountMap::ATTR_BANK_ACCOUNT_NAME);
			elem[sslit("bankName")]          = AccountMap::get_attribute(account_id, AccountMap::ATTR_BANK_NAME);
			elem[sslit("bankAccountNumber")] = AccountMap::get_attribute(account_id, AccountMap::ATTR_BANK_ACCOUNT_NUMBER);
			elem[sslit("bankSwiftCode")]     = AccountMap::get_attribute(account_id, AccountMap::ATTR_BANK_SWIFT_CODE);
			requests.emplace_back(std::move(elem));
		}
		ret[sslit("requests")] = std::move(requests);
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
