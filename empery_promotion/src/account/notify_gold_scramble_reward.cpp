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
	const auto &login_name = params.at("loginName");
	const auto gold_coins = boost::lexical_cast<std::uint64_t>(params.at("goldCoins"));
	const auto account_balance = boost::lexical_cast<std::uint64_t>(params.at("accountBalance"));
	const auto game_begin_time = boost::lexical_cast<std::uint64_t>(params.at("gameBeginTime"));
	const auto gold_coins_in_pot = boost::lexical_cast<std::uint64_t>(params.at("goldCoinsInPot"));
	const auto account_balance_in_pot = boost::lexical_cast<std::uint64_t>(params.at("accountBalanceInPot"));

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	if((info.banned_until != 0) && (Poseidon::get_utc_time() < info.banned_until)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Account is banned";
		return ret;
	}

	std::vector<ItemTransactionElement> transaction;
	if(gold_coins != 0){
		transaction.emplace_back(info.account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_GOLD_COINS, gold_coins,
			Events::ItemChanged::R_GOLD_SCRAMBLE_REWARD, game_begin_time, gold_coins_in_pot, account_balance_in_pot, std::string());
	}
	if(account_balance != 0){
		transaction.emplace_back(info.account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, account_balance,
			Events::ItemChanged::R_GOLD_SCRAMBLE_REWARD, game_begin_time, gold_coins_in_pot, account_balance_in_pot, std::string());
	}
	ItemMap::commit_transaction(transaction.data(), transaction.size());

	ret[sslit("goldCoins")] = ItemMap::get_count(info.account_id, ItemIds::ID_GOLD_COINS);
	ret[sslit("accountBalance")] = ItemMap::get_count(info.account_id, ItemIds::ID_ACCOUNT_BALANCE);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
