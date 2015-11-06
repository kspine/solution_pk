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
	const auto &loginName = params.at("loginName");
	const auto itemId = boost::lexical_cast<ItemId>(params.at("itemId"));
	const auto countToAdd = boost::lexical_cast<boost::uint64_t>(params.at("countToAdd"));
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;
	auto info = AccountMap::getByLoginName(loginName);
	if(Poseidon::hasNoneFlagsOf(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(info.accountId, ItemTransactionElement::OP_ADD, itemId, countToAdd,
		Events::ItemChanged::R_ADMIN_OPERATION, 0, 0, 0, remarks);
	ItemMap::commitTransaction(transaction.data(), transaction.size());

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
