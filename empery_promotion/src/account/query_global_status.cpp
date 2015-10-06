#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/global_status.hpp"
#include "../msg/err_account.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("queryGlobalStatus", /* session */, /* params */){
	Poseidon::JsonObject ret;

	ret[sslit("createdTime")]        = GlobalStatus::get(GlobalStatus::SLOT_SERVER_CREATED_TIME);
	ret[sslit("firstBalancingTime")] = GlobalStatus::get(GlobalStatus::SLOT_FIRST_BALANCING_TIME);
	ret[sslit("accCardUnitPrice")]   = GlobalStatus::get(GlobalStatus::SLOT_ACC_CARD_UNIT_PRICE);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
