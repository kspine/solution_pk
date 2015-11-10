#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/item_map.hpp"
#include "../item_transaction_element.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("notifyGoldScrambleReward", session, params){
	const auto &loginName = params.at("loginName");
	const auto goldCoins = boost::lexical_cast<boost::uint64_t>(params.at("goldCoins"));
	const auto accountBalance = boost::lexical_cast<boost::uint64_t>(params.at("accountBalance"));
	const auto gameBeginTime = boost::lexical_cast<boost::uint64_t>(params.at("gameBeginTime"));
	const auto goldCoinsInPot = boost::lexical_cast<boost::uint64_t>(params.at("goldCoinsInPot"));
	const auto accountBalanceInPot = boost::lexical_cast<boost::uint64_t>(params.at("accountBalanceInPot"));

	Poseidon::JsonObject ret;
	auto info = AccountMap::getByLoginName(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	if((info.bannedUntil != 0) && (Poseidon::getUtcTime() < info.bannedUntil)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Account is banned";
		return ret;
	}

	std::vector<ItemTransactionElement> transaction;
	if(goldCoins != 0){
		transaction.emplace_back(info.accountId, ItemTransactionElement::OP_ADD, ItemIds::ID_GOLD_COINS, goldCoins,
			Events::ItemChanged::R_GOLD_SCRAMBLE_REWARD, gameBeginTime, goldCoinsInPot, accountBalanceInPot, std::string());
	}
	if(goldCoins != 0){
		transaction.emplace_back(info.accountId, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, accountBalance,
			Events::ItemChanged::R_GOLD_SCRAMBLE_REWARD, gameBeginTime, goldCoinsInPot, accountBalanceInPot, std::string());
	}
	ItemMap::commitTransaction(transaction.data(), transaction.size());

	ret[sslit("goldCoins")] = ItemMap::getCount(info.accountId, ItemIds::ID_GOLD_COINS);
	ret[sslit("accountBalance")] = ItemMap::getCount(info.accountId, ItemIds::ID_ACCOUNT_BALANCE);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
