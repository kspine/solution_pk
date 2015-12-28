#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/item_map.hpp"
#include "../msg/err_account.hpp"
#include "../item_ids.hpp"
#include "../item_transaction_element.hpp"
#include "../events/item.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("addAccountItems", session, params){
	const auto &login_name = params.at("loginName");
	const auto item_id = boost::lexical_cast<ItemId>(params.at("itemId"));
	const auto count_to_add = boost::lexical_cast<std::uint64_t>(params.at("countToAdd"));
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(info.account_id, ItemTransactionElement::OP_ADD, item_id, count_to_add,
		Events::ItemChanged::R_ADMIN_OPERATION, 0, 0, 0, remarks);
	ItemMap::commit_transaction(transaction.data(), transaction.size());

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
