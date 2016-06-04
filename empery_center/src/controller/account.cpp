#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../msg/ts_account.hpp"
#include "../singletons/account_map.hpp"
#include "../transaction_element.hpp"
#include "../item_box.hpp"
#include "../singletons/item_box_map.hpp"

namespace EmperyCenter {

CONTROLLER_SERVLET(Msg::TS_AccountAddItems, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	const auto item_box = ItemBoxMap::require(account_uuid);

	std::vector<ItemTransactionElement> transaction;
	transaction.reserve(req.items.size());
	for(auto it = req.items.begin(); it != req.items.end(); ++it){
		transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemId(it->item_id), it->count,
			ReasonId(req.reason), req.param1, req.param2, req.param3);
	}
	item_box->commit_transaction(transaction, false);

	return Response();
}

}
