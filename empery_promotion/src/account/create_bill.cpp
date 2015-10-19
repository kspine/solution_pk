#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/bill.hpp"
#include "../utilities.hpp"
#include "../bill_states.hpp"
#include <poseidon/http/utilities.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("createBill", /* session */, params){
	const auto &loginName = params.at("loginName");
	const auto amount = boost::lexical_cast<boost::uint64_t>(params.at("amount"));
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;
	auto info = AccountMap::getByLoginName(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	auto serialPrefix = getConfig<std::string>("bill_serial_prefix", "BI");

	auto serial = generateBillSerial(serialPrefix);
	const auto createdTime = Poseidon::getLocalTime();
	const auto obj = boost::make_shared<MySql::Promotion_Bill>(
		serial, createdTime, amount, info.accountId.get(), BillStates::ST_NEW, std::string(), remarks);
	obj->asyncSave(true);
	LOG_EMPERY_PROMOTION_INFO("Created bill: serial = ", serial, ", amount = ", amount, ", accountId = ", info.accountId);

	ret[sslit("serial")] = std::move(serial);
	ret[sslit("phoneNumber")] = std::move(info.phoneNumber);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
