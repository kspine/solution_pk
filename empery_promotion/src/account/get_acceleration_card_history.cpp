#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/item_changed.hpp"
#include "../mysql/sum_rows.hpp"
#include "../item_ids.hpp"
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("getAccelerationCardHistory", session, params){
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

	std::vector<boost::shared_ptr<EmperyPromotionLog::MySql::PromotionLog_ItemChanged>> objs;
	std::ostringstream oss;
	oss <<"SELECT ";
	if(brief_mode.empty()){
		oss <<"* ";
	} else {
		oss <<"SUM(CONVERT(`new_count`, SIGNED) - CONVERT(`old_count`, SIGNED)) AS `sum`, COUNT(*) AS `rows` ";
	}
	oss <<"FROM `PromotionLog_ItemChanged` WHERE `item_id` = " <<ItemIds::ID_ACCELERATION_CARDS <<" ";
	if(!time_begin.empty()){
		char str[256];
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<std::uint64_t>(time_begin), false);
		oss <<"AND " <<Poseidon::MySql::StringEscaper(str) <<" <= `timestamp` ";
		Poseidon::format_time(str, sizeof(str), boost::lexical_cast<std::uint64_t>(time_end), false);
		oss <<"AND `timestamp` < " <<Poseidon::MySql::StringEscaper(str) <<" ";
	}
	if(!reason.empty()){
		auto enum_reason = boost::lexical_cast<std::uint32_t>(reason);
		oss <<"AND `reason` = " <<enum_reason <<" ";
	}
	if(!login_name.empty()){
		oss <<"AND `account_id` = " <<account_id <<" ";
	}
	if(brief_mode.empty()){
		oss <<"ORDER BY `timestamp` DESC ";
		if(!count.empty()){
			oss <<"LIMIT ";
			if(!begin.empty()){
				auto num_begin = boost::lexical_cast<std::uint64_t>(begin);
				oss <<num_begin <<", ";
			}
			auto num_count = boost::lexical_cast<std::uint64_t>(count);
			oss <<num_count;
		}
		auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[&](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<EmperyPromotionLog::MySql::PromotionLog_ItemChanged>();
				obj->fetch(conn);
				objs.emplace_back(std::move(obj));
			}, "PromotionLog_Item", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);

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
			elem[sslit("oldCount")] = obj->get_old_count();
			elem[sslit("newCount")] = obj->get_new_count();
			elem[sslit("reason")] = obj->get_reason();
			elem[sslit("param1")] = obj->get_param1();
			elem[sslit("param2")] = obj->get_param2();
			elem[sslit("param3")] = obj->get_param3();
			elem[sslit("loginName")] = std::move(info.login_name);
			elem[sslit("nick")] = std::move(info.nick);
			history.emplace_back(std::move(elem));
		}
		ret[sslit("history")] = std::move(history);
	} else {
		const auto obj = boost::make_shared<MySql::Promotion_SumRows>();
		auto promise = Poseidon::MySqlDaemon::enqueue_for_loading(obj, oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
		ret[sslit("sum")] = obj->get_sum();
		ret[sslit("rows")] = obj->get_rows();
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
