#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../item_ids.hpp"
#include "../item_transaction_element.hpp"
#include "../events/item.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("removeAccountItems", session, params){
	const auto &login_name = params.at("loginName");
	const auto item_id = boost::lexical_cast<ItemId>(params.at("itemId"));
	const auto count_to_remove = boost::lexical_cast<boost::uint64_t>(params.at("countToRemove"));
	const auto &saturated = params.get("saturated");
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	std::vector<ItemTransactionElement> transaction;
	auto operation = ItemTransactionElement::OP_REMOVE;
	if(!saturated.empty()){
		operation = ItemTransactionElement::OP_REMOVE_SATURATED;
	}
	transaction.emplace_back(info.account_id, operation, item_id, count_to_remove,
		Events::ItemChanged::R_ADMIN_OPERATION, 0, 0, 0, remarks);
	const auto insufficient_item_id = ItemMap::commit_transaction_nothrow(transaction.data(), transaction.size());
	if(insufficient_item_id){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ITEMS;
		ret[sslit("errorMessage")] = "No enough items";
		return ret;
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
