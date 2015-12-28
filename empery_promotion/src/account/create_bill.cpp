#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/bill.hpp"
#include "../utilities.hpp"
#include "../bill_states.hpp"
#include <poseidon/http/utilities.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("createBill", session, params){
	const auto &login_name = params.at("loginName");
	const auto amount = boost::lexical_cast<std::uint64_t>(params.at("amount"));
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	auto serial_prefix = get_config<std::string>("bill_serial_prefix", "BI");

	auto serial = generate_bill_serial(serial_prefix);
	const auto created_time = Poseidon::get_utc_time();
	const auto obj = boost::make_shared<MySql::Promotion_Bill>(
		serial, created_time, amount, info.account_id.get(), BillStates::ST_NEW, std::string(), remarks);
	obj->async_save(true);
	LOG_EMPERY_PROMOTION_INFO("Created bill: serial = ", serial, ", amount = ", amount, ", account_id = ", info.account_id);

	ret[sslit("serial")] = std::move(serial);
	ret[sslit("phoneNumber")] = std::move(info.phone_number);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
