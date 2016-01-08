#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/bill.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("queryBillState", session, params){
	const auto &serial = params.at("ordId");

	Poseidon::JsonObject ret;
	std::vector<boost::shared_ptr<MySql::Promotion_Bill>> objs;
	std::ostringstream oss;
	oss <<"SELECT * FROM `Promotion_Bill` WHERE `serial` = " <<Poseidon::MySql::StringEscaper(serial) <<" LIMIT 1";
	MySql::Promotion_Bill::batch_load(objs, oss.str());
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
