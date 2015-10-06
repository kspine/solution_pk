#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("setAccountAttributes", /* session */, params){
	const auto &loginName         = params.at("loginName");
	const auto &phoneNumber       = params.get("phoneNumber");
	const auto &nick              = params.get("nick");
	const auto &bannedUntil       = params.get("bannedUntil");
	const auto &gender            = params.get("gender");
	const auto &country           = params.get("country");
	const auto &bankAccountName   = params.get("bankAccountName");
	const auto &bankName          = params.get("bankName");
	const auto &bankAccountNumber = params.get("bankAccountNumber");
	const auto &bankSwiftCode     = params.get("bankSwiftCode");
	const auto &remarks           = params.get("remarks");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	if(!phoneNumber.empty()){
		AccountMap::setPhoneNumber(info.accountId, std::move(phoneNumber));
	}
	if(!nick.empty()){
		AccountMap::setNick(info.accountId, std::move(nick));
	}
	if(!bannedUntil.empty()){
		AccountMap::setBannedUntil(info.accountId, boost::lexical_cast<boost::uint64_t>(bannedUntil));
	}

	if(!gender.empty()){
		AccountMap::setAttribute(info.accountId, AccountMap::ATTR_GENDER, std::move(gender));
	}
	if(!country.empty()){
		AccountMap::setAttribute(info.accountId, AccountMap::ATTR_COUNTRY, std::move(country));
	}
	if(!bankAccountName.empty()){
		AccountMap::setAttribute(info.accountId, AccountMap::ATTR_BANK_ACCOUNT_NAME, std::move(bankAccountName));
	}
	if(!bankName.empty()){
		AccountMap::setAttribute(info.accountId, AccountMap::ATTR_BANK_NAME, std::move(bankName));
	}
	if(!bankAccountNumber.empty()){
		AccountMap::setAttribute(info.accountId, AccountMap::ATTR_BANK_ACCOUNT_NUMBER, std::move(bankAccountNumber));
	}
	if(!bankSwiftCode.empty()){
		AccountMap::setAttribute(info.accountId, AccountMap::ATTR_BANK_SWIFT_CODE, std::move(bankSwiftCode));
	}
	if(!remarks.empty()){
		AccountMap::setAttribute(info.accountId, AccountMap::ATTR_REMARKS, std::move(remarks));
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
