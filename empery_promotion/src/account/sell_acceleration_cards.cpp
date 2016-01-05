#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../utilities.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("sellAccelerationCards", session, params){
	const auto &login_name = params.at("loginName");
	const auto unit_price = boost::lexical_cast<std::uint64_t>(params.at("unitPrice"));
	const auto cards_to_sell = boost::lexical_cast<std::uint64_t>(params.at("cardsToSell"));

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	if(cards_to_sell == 0){
		ret[sslit("errorCode")] = (int)Msg::ERR_ZERO_CARD_COUNT;
		ret[sslit("errorMessage")] = "cardsToSell set to zero";
		return ret;
	}

	const auto cards_sold = sell_acceleration_cards(info.account_id, unit_price, cards_to_sell);
	if(cards_sold == 0){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_MORE_ACCELERATION_CARDS;
		ret[sslit("errorMessage")] = "No more acceleration cards on the earth";
		return ret;
	}

	ret[sslit("cardsSold")] = cards_sold;

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
