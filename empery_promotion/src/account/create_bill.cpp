#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/chinapnr_sign_daemon.hpp"
#include "../msg/err_account.hpp"
#include "../mysql/bill.hpp"
#include "../utilities.hpp"
#include "../bill_states.hpp"
#include <poseidon/http/utilities.hpp>

namespace EmperyPromotion {

ACCOUNT_SERVLET("createBill", /* session */, params){
	const auto &loginName = params.at("loginName");
	const auto amount = boost::lexical_cast<boost::uint64_t>(params.at("amount"));
	const auto &serverIp = params.at("serverIp");
	const auto &retUrl = params.at("retUrl");
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	auto serialPrefix = getConfig<std::string>("bill_serial_prefix", "BI");

	auto bgPort = getConfig<unsigned>   ("chinapnr_http_server_port", 6215);
	auto bgCert = getConfig<std::string>("chinapnr_http_server_certificate");
	auto bgAuth = getConfig<std::string>("chinapnr_http_auth_user_pass", "");
	auto bgPath = getConfig<std::string>("chinapnr_http_server_path", "/");

	auto merId   = getConfig<std::string>("chinapnr_merid");
//	auto privKey = getConfig<std::string>("chinapnr_private_key");
//	auto pubKey  = getConfig<std::string>("chinapnr_public_key");

	auto serial = generateBillSerial(serialPrefix);
	const auto createdTime = Poseidon::getUtcTime() + 8 * 3600 * 1000; // XXX 转换到北京时间。

	auto usrMp = std::move(info.phoneNumber);
	if((usrMp.size() >= 4) && (std::memcmp(usrMp.data(), "+86-", 4) == 0)){ // XXX 去掉国际区号。
		usrMp.erase(0, 4);
	}

	char str[256];
	auto len = (unsigned)std::sprintf(str, "%llu.%02u", (unsigned long long)(amount / 100), (unsigned)(amount % 100));
	auto amountStr = std::string(str, len);

	const auto obj = boost::make_shared<MySql::Promotion_Bill>(
		serial, createdTime, amount, info.accountId.get(), BillStates::ST_NEW, std::string(), remarks);
	obj->asyncSave(true);
	LOG_EMPERY_PROMOTION_INFO("Created bill: serial = ", serial, ", amount = ", amount, ", accountId = ", info.accountId);

	std::ostringstream oss;
	oss <<(bgCert.empty() ? "http://" : "https://") <<serverIp <<':' <<bgPort <<bgPath <<"/settle";
	auto bgRetUrl = oss.str();

	auto chkValue = ChinaPnrSignDaemon::sign(merId, serial, createdTime, amount, retUrl, usrMp, bgRetUrl);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	ret[sslit("merId")] = std::move(merId);
	ret[sslit("ordId")] = std::move(serial);
	ret[sslit("ordAmt")] = std::move(amountStr);
	ret[sslit("usrMp")] = std::move(usrMp);
	ret[sslit("retUrl")] = std::move(retUrl);
	ret[sslit("bgRetUrl")] = std::move(bgRetUrl);
	ret[sslit("chkValue")] = std::move(chkValue);
	return ret;
}

}
