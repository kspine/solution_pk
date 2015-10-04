#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/string.hpp>
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../item_transaction_element.hpp"
#include "../msg/err_account.hpp"
#include "../data/promotion.hpp"
#include "../utilities.hpp"
#include "../events/account.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("create", session, params){
	const auto &payerLoginName = params.at("payerLoginName");
	const auto &dealPassword = params.get("dealPassword");
	const auto &loginName = params.at("loginName");
	const auto &nick = params.at("nick");
	const auto level = boost::lexical_cast<boost::uint64_t>(params.at("level"));
	const auto &gender = params.at("gender");
	const auto &country = params.at("country");
	const auto &password = params.at("password");
	const auto &phoneNumber = params.at("phoneNumber");
	const auto &bankAccountName = params.at("bankAccountName");
	const auto &bankName = params.at("bankName");
	const auto &bankAccountNumber = params.at("bankAccountNumber");
	const auto &referrerLoginName = params.get("referrerLoginName");
	const auto &bankSwiftCode = params.get("bankSwiftCode");
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;

	auto payerInfo = AccountMap::get(payerLoginName);
	if(Poseidon::hasNoneFlagsOf(payerInfo.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_PAYER;
		ret[sslit("errorMessage")] = "Payer is not found";
		return ret;
	}
	if(level != 0){
		if(AccountMap::getPasswordHash(dealPassword) != payerInfo.dealPasswordHash){
			ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_DEAL_PASSWORD;
			ret[sslit("errorMessage")] = "Deal password is incorrect";
			return ret;
		}
	}
	const auto localNow = Poseidon::getLocalTime();
	if((payerInfo.bannedUntil != 0) && (localNow < payerInfo.bannedUntil)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Payer is banned";
		return ret;
	}

	auto referrerInfo = referrerLoginName.empty() ? payerInfo : AccountMap::get(referrerLoginName);
	if(Poseidon::hasNoneFlagsOf(referrerInfo.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_REFERRER;
		ret[sslit("errorMessage")] = "Referrer is not found";
		return ret;
	}
	if((referrerInfo.bannedUntil != 0) && (localNow < referrerInfo.bannedUntil)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Referrer is banned";
		return ret;
	}

	boost::shared_ptr<const Data::Promotion> promotionData;
	if(level != 0){
		promotionData = Data::Promotion::get(level);
		if(!promotionData){
			ret[sslit("errorCode")] = (int)Msg::ERR_UNKNOWN_ACCOUNT_LEVEL;
			ret[sslit("errorMessage")] = "Account level is not found";
			return ret;
		}
	}

	if(AccountMap::has(loginName)){
		ret[sslit("errorCode")] = (int)Msg::ERR_DUPLICATE_LOGIN_NAME;
		ret[sslit("errorMessage")] = "Another account with the same login name already exists";
		return ret;
	}

	std::vector<AccountMap::AccountInfo> tempInfos;
	AccountMap::getByPhoneNumber(tempInfos, phoneNumber);
	if(!tempInfos.empty()){
		ret[sslit("errorCode")] = (int)Msg::ERR_DUPLICATE_PHONE_NUMBER;
		ret[sslit("errorMessage")] = "Another account with the same phone number already exists";
		return ret;
	}

	const auto newAccountId = AccountMap::create(loginName, phoneNumber, nick, password, password, referrerInfo.accountId, 0);
	AccountMap::setAttribute(newAccountId, AccountMap::ATTR_GENDER, gender);
	AccountMap::setAttribute(newAccountId, AccountMap::ATTR_COUNTRY, country);
//	AccountMap::setAttribute(newAccountId, AccountMap::ATTR_PHONE_NUMBER, phoneNumber);
	AccountMap::setAttribute(newAccountId, AccountMap::ATTR_BANK_ACCOUNT_NAME, bankAccountName);
	AccountMap::setAttribute(newAccountId, AccountMap::ATTR_BANK_NAME, bankName);
	AccountMap::setAttribute(newAccountId, AccountMap::ATTR_BANK_ACCOUNT_NUMBER, bankAccountNumber);
	AccountMap::setAttribute(newAccountId, AccountMap::ATTR_BANK_SWIFT_CODE, bankSwiftCode);
	AccountMap::setAttribute(newAccountId, AccountMap::ATTR_REMARKS, remarks);

	Poseidon::asyncRaiseEvent(
		boost::make_shared<Events::AccountCreated>(newAccountId, session->getRemoteInfo().ip));

	const auto initGoldCoinArray = Poseidon::explode<boost::uint64_t>(',',
	                               getConfig<std::string>("init_gold_coins_array", "100,50,50"));
	std::vector<ItemTransactionElement> transaction;
	transaction.reserve(initGoldCoinArray.size());
	auto addGoldCoinsToWhom = newAccountId;
	for(auto it = initGoldCoinArray.begin(); it != initGoldCoinArray.end(); ++it){
		transaction.emplace_back(addGoldCoinsToWhom, ItemTransactionElement::OP_ADD, ItemIds::ID_GOLD_COINS, *it,
			Events::ItemChanged::R_CREATE_ACCOUNT, newAccountId.get(), payerInfo.accountId.get(), level, remarks);

		const auto info = AccountMap::require(addGoldCoinsToWhom);
		addGoldCoinsToWhom = info.referrerId;
		if(!addGoldCoinsToWhom){
			break;
		}
	}
	ItemMap::commitTransaction(transaction.data(), transaction.size());

	if(promotionData){
		const auto result = tryUpgradeAccount(newAccountId, payerInfo.accountId, true, promotionData, remarks);
		ret[sslit("balanceToConsume")] = result.second;
		if(!result.first){
			ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE;
			ret[sslit("errorMessage")] = "No enough account balance";
			return ret;
		}
		accumulateBalanceBonus(newAccountId, payerInfo.accountId, result.second);
	} else {
		ret[sslit("balanceToConsume")] = 0;
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
