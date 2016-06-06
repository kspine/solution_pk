#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../msg/ts_account.hpp"
#include "../singletons/account_map.hpp"
#include "../transaction_element.hpp"
#include "../item_box.hpp"
#include "../singletons/item_box_map.hpp"
#include "../player_session.hpp"
#include "../singletons/player_session_map.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>

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
			ReasonId(it->reason), it->param1, it->param2, it->param3);
	}
	item_box->commit_transaction(transaction, false);

	return Response();
}

CONTROLLER_SERVLET(Msg::TS_AccountInvalidateAccount, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::reload(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		session->shutdown(req.reason, req.param.c_str());
	}

	const auto promise = Poseidon::MySqlDaemon::enqueue_for_waiting_for_all_async_operations();
	Poseidon::JobDispatcher::yield(promise, false);

	return Response();
}

}
