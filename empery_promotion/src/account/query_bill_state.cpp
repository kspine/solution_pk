#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/bill.hpp"
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("queryBillState", session, params){
	const auto &serial = params.at("ordId");

	Poseidon::JsonObject ret;
	std::vector<boost::shared_ptr<MySql::Promotion_Bill>> objs;
	std::ostringstream oss;
	oss <<"SELECT * FROM `Promotion_Bill` WHERE `serial` = " <<Poseidon::MySql::StringEscaper(serial) <<" LIMIT 1";
	auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
		[&](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
			auto obj = boost::make_shared<MySql::Promotion_Bill>();
			obj->fetch(conn);
			objs.emplace_back(std::move(obj));
		}, "Promotion_Bill", oss.str());
	Poseidon::JobDispatcher::yield(promise, true);
	if(objs.empty()){
		LOG_EMPERY_PROMOTION_DEBUG("No rows returned");
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_BILL;
		ret[sslit("errorMessage")] = "Bill is not found";
		return ret;
	}
	const auto obj = std::move(objs.front());

	ret[sslit("state")] = obj->get_state();

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
