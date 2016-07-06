#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/ts_account.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../transaction_element.hpp"
#include "../item_box.hpp"
#include "../item_ids.hpp"
#include "../singletons/item_box_map.hpp"
#include "../tax_record_box.hpp"
#include "../singletons/tax_record_box_map.hpp"
#include "../player_session.hpp"
#include "../singletons/player_session_map.hpp"
#include "../singletons/battle_record_box_map.hpp"
#include "../singletons/task_box_map.hpp"
#include "../singletons/auction_center_map.hpp"
#include "../singletons/mail_box_map.hpp"

namespace EmperyCenter {

CONTROLLER_SERVLET(Msg::TS_AccountSendPromotionBonus, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	const auto item_box = ItemBoxMap::require(account_uuid);
	const auto tax_record_box = TaxRecordBoxMap::require(account_uuid);

	const auto taxer_uuid = AccountUuid(req.taxer_uuid);
	const auto amount = req.amount;
	const auto reason = ReasonId(req.reason);
	const auto param1 = req.param1, param2 = req.param2, param3 = req.param3;

	const auto utc_now = Poseidon::get_utc_time();
	const auto old_amount = item_box->get(ItemIds::ID_GOLD).count;
	const auto new_amount = checked_add(old_amount, amount);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemIds::ID_GOLD, amount,
		ReasonId(reason), param1, param2, param3);
	item_box->commit_transaction(transaction, false,
		[&]{ tax_record_box->push(utc_now, taxer_uuid, reason, old_amount, new_amount); });

	return Response();
}

CONTROLLER_SERVLET(Msg::TS_AccountInvalidate, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::forced_reload(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		session->shutdown(req.reason, req.param.c_str());
	}

	BattleRecordBoxMap::unload(account_uuid);
	BattleRecordBoxMap::unload_crate(account_uuid);
	TaskBoxMap::unload(account_uuid);
	AuctionCenterMap::unload(account_uuid);
	MailBoxMap::unload(account_uuid);
	Poseidon::MySqlDaemon::enqueue_for_waiting_for_all_async_operations();

	return Response();
}

}
