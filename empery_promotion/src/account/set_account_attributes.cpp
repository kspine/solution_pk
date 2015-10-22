#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../data/promotion.hpp"
#include "../msg/err_account.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("setAccountAttributes", /* session */, params){
	const auto &loginName      = params.at("loginName");
	auto newLoginName          = params.get("newLoginName");
	auto phoneNumber           = params.get("phoneNumber");
	auto nick                  = params.get("nick");
	auto password              = params.get("password");
	auto dealPassword          = params.get("dealPassword");
	auto bannedUntil           = params.get("bannedUntil");
	auto gender                = params.get("gender");
	auto country               = params.get("country");
	auto bankAccountName       = params.get("bankAccountName");
	auto bankName              = params.get("bankName");
	auto bankAccountNumber     = params.get("bankAccountNumber");
	auto bankSwiftCode         = params.get("bankSwiftCode");
	auto remarks               = params.get("remarks");
	auto level                 = params.get("level");
	auto maxVisibleSubordDepth = params.get("maxVisibleSubordDepth");

	Poseidon::JsonObject ret;
	auto info = AccountMap::getByLoginName(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	if(!newLoginName.empty()){
		auto tempInfo = AccountMap::getByLoginName(newLoginName);
		if(Poseidon::hasAnyFlagsOf(tempInfo.flags, AccountMap::FL_VALID) && (tempInfo.accountId != info.accountId)){
			ret[sslit("errorCode")] = (int)Msg::ERR_DUPLICATE_LOGIN_NAME;
			ret[sslit("errorMessage")] = "Another account with the same login name already exists";
			return ret;
		}
	}

	if(!phoneNumber.empty()){
		std::vector<AccountMap::AccountInfo> tempInfos;
		AccountMap::getByPhoneNumber(tempInfos, phoneNumber);
		if(!tempInfos.empty() && (tempInfos.front().accountId != info.accountId)){
			ret[sslit("errorCode")] = (int)Msg::ERR_DUPLICATE_PHONE_NUMBER;
			ret[sslit("errorMessage")] = "Another account with the same phone number already exists";
			return ret;
		}
	}
	if(!level.empty()){
		const auto newPromotionData = Data::Promotion::get(boost::lexical_cast<boost::uint64_t>(level));
		if(!newPromotionData){
			ret[sslit("errorCode")] = (int)Msg::ERR_UNKNOWN_ACCOUNT_LEVEL;
			ret[sslit("errorMessage")] = "Account level is not found";
			return ret;
		}
		level = boost::lexical_cast<std::string>(newPromotionData->level);
	}
	if(!maxVisibleSubordDepth.empty()){
		const auto depth = boost::lexical_cast<boost::uint64_t>(maxVisibleSubordDepth);
		maxVisibleSubordDepth = boost::lexical_cast<std::string>(depth);
	}

	if(!newLoginName.empty()){
		AccountMap::setLoginName(info.accountId, std::move(newLoginName));
	}
	if(!phoneNumber.empty()){
		AccountMap::setPhoneNumber(info.accountId, std::move(phoneNumber));
	}
	if(!nick.empty()){
		AccountMap::setNick(info.accountId, std::move(nick));
	}
	if(!password.empty()){
		AccountMap::setPassword(info.accountId, std::move(password));
	}
	if(!dealPassword.empty()){
		AccountMap::setDealPassword(info.accountId, std::move(dealPassword));
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
	if(!level.empty()){
		AccountMap::setAttribute(info.accountId, AccountMap::ATTR_ACCOUNT_LEVEL, std::move(level));
	}
	if(!maxVisibleSubordDepth.empty()){
		AccountMap::setAttribute(info.accountId, AccountMap::ATTR_MAX_VISIBLE_SUBORD_DEPTH, std::move(maxVisibleSubordDepth));
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
