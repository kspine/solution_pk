#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/cs_item.hpp"
#include "../msg/err_item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../data/item.hpp"
#include "../transaction_element.hpp"
#include "../item_ids.hpp"
#include "../reason_ids.hpp"
#include "../data/global.hpp"
#include "../singletons/simple_http_client_daemon.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_ItemGetAllItems, account, session, /* req */){
	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
	item_box->pump_status();

	item_box->synchronize_with_player(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_ItemTradeFromRecharge, account, session, req){
	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
	item_box->pump_status();

	const auto repeat_count = req.repeat_count;
	if(repeat_count == 0){
		return Response(Msg::ERR_ZERO_REPEAT_COUNT);
	}
	const auto recharge_id = RechargeId(req.recharge_id);
	const auto recharge_data = Data::ItemRecharge::get(recharge_id);
	if(!recharge_data){
		return Response(Msg::ERR_NO_SUCH_RECHARGE_ID) <<recharge_id;
	}
	const auto trade_data = Data::ItemTrade::require(recharge_data->trade_id);

	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, repeat_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true);
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_ItemTradeFromShop, account, session, req){
	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
	item_box->pump_status();

	const auto repeat_count = req.repeat_count;
	if(repeat_count == 0){
		return Response(Msg::ERR_ZERO_REPEAT_COUNT);
	}
	const auto shop_id = ShopId(req.shop_id);
	const auto shop_data = Data::ItemShop::get(shop_id);
	if(!shop_data){
		return Response(Msg::ERR_NO_SUCH_SHOP_ID) <<shop_id;
	}
	const auto trade_data = Data::ItemTrade::require(shop_data->trade_id);

	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, repeat_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true);
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_ItemUseItem, account, session, req){
	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
	item_box->pump_status();

	const auto repeat_count = req.repeat_count;
	if(repeat_count == 0){
		return Response(Msg::ERR_ZERO_REPEAT_COUNT);
	}
	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::require(item_id);
	const auto trade_id = item_data->use_as_trade_id;
	if(!trade_id){
		return Response(Msg::ERR_ITEM_NOT_USABLE) <<item_id;
	}
	const auto trade_data = Data::ItemTrade::require(trade_id);

	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, repeat_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false);
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

namespace {
	std::string g_promotion_host    = "localhost";
	unsigned    g_promotion_port    = 6121;
	bool        g_promotion_use_ssl = false;
	std::string g_promotion_auth    = { };
	std::string g_promotion_path    = { };

	MODULE_RAII_PRIORITY(/* handles */, 1000){
		get_config(g_promotion_host,    "promotion_server_host");
		get_config(g_promotion_port,    "promotion_server_port");
		get_config(g_promotion_use_ssl, "promotion_server_use_ssl");
		get_config(g_promotion_auth,    "promotion_server_auth_user_pass");
		get_config(g_promotion_path,    "promotion_server_path");
	}
}

PLAYER_SERVLET(Msg::CS_ItemBuyAccelerationCards, account, session, req){
	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
	item_box->pump_status();

	const auto repeat_count = req.repeat_count;
	if(repeat_count == 0){
		return Response(Msg::ERR_ZERO_REPEAT_COUNT);
	}
	auto item_id = ItemIds::ID_DIAMONDS;
	if(req.use_gold != 0){
		item_id = ItemIds::ID_GOLD;
	}

	const auto unit_price = Data::Global::as_unsigned(Data::Global::SLOT_ACCELERATION_CARD_UNIT_PRICE);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, checked_mul(repeat_count, unit_price),
		ReasonIds::ID_BUY_ACCELERATION_CARD, account->get_promotion_level(), 0, 0);
	transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemIds::ID_ACCELERATION_CARD, repeat_count,
		ReasonIds::ID_BUY_ACCELERATION_CARD, account->get_promotion_level(), 0, 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false);
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	Poseidon::OptionalMap get_params;
	get_params.set(sslit("loginName"),   account->get_login_name());
	get_params.set(sslit("unitPrice"),   boost::lexical_cast<std::string>(unit_price));
	get_params.set(sslit("cardsToSell"), boost::lexical_cast<std::string>(repeat_count));

	const auto entity = SimpleHttpClientDaemon::sync_request(g_promotion_host, g_promotion_port, g_promotion_use_ssl,
		Poseidon::Http::V_GET, g_promotion_path + "/sellAccelerationCards", std::move(get_params), g_promotion_auth);
	LOG_EMPERY_CENTER_DEBUG("Received response from promotion server: entity = ", entity.dump());

	return Response();
}

PLAYER_SERVLET(Msg::CS_ItemUseItemBox, account, session, req){
	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
	item_box->pump_status();

	const auto repeat_count = req.repeat_count;
	if(repeat_count == 0){
		return Response(Msg::ERR_ZERO_REPEAT_COUNT);
	}
	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::require(item_id);
	if(item_data->type.first != Data::Item::CAT_ITEM_BOX){
		return Response(Msg::ERR_ITEM_TYPE_MISMATCH) <<(unsigned)Data::Item::CAT_ITEM_BOX;
	}
	const auto new_item_id = ItemId(item_data->type.second);
	const auto count_to_consume = req.repeat_count;
	const auto count_to_add = checked_mul(count_to_consume, item_data->value);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, count_to_consume,
		ReasonIds::ID_UNPACK, 0, item_id.get(), count_to_consume);
	transaction.emplace_back(ItemTransactionElement::OP_ADD, new_item_id, count_to_add,
		ReasonIds::ID_UNPACK, 0, item_id.get(), count_to_consume);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false);
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_ItemTrade, account, session, req){
	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
	item_box->pump_status();

	const auto repeat_count = req.repeat_count;
	if(repeat_count == 0){
		return Response(Msg::ERR_ZERO_REPEAT_COUNT);
	}
	const auto trade_id = TradeId(req.trade_id);
	const auto trade_data = Data::ItemTrade::get(trade_id);
	if(!trade_data){
		return Response(Msg::ERR_NO_SUCH_TRADE_ID) <<trade_id;
	}

	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, repeat_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true);
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_ItemDungeonTrade, account, session, req){
	const auto item_box = ItemBoxMap::require(account->get_account_uuid());
	item_box->pump_status();

	const auto repeat_count = req.repeat_count;
	if(repeat_count == 0){
		return Response(Msg::ERR_ZERO_REPEAT_COUNT);
	}
	
	if(req.trade_id != 2804026){
		return Response(Msg::ERR_NOT_DUNGEON_TRADE);
	}
	
	const auto trade_id = TradeId(req.trade_id);
	const auto trade_data = Data::ItemTrade::get(trade_id);
	if(!trade_data){
		return Response(Msg::ERR_NO_SUCH_TRADE_ID) <<trade_id;
	}
	const auto item_have_buy_info = item_box->get(ItemIds::ID_DUNGEON_HAVE_BUY_TIMES);
	
	const auto dungeon_trade_param = Data::Global::as_unsigned(Data::Global::SLOT_ITEM_DUNGEON_TRAD_PARAM);
	std::vector<ItemTransactionElement> transaction;
	for(unsigned i = 0; i < repeat_count; ++i){
		Data::unpack_item_trade(transaction, trade_data, 1, req.ID,static_cast<std::uint64_t>(dungeon_trade_param*(item_have_buy_info.count + i)));
	}
	transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemIds::ID_DUNGEON_HAVE_BUY_TIMES, repeat_count,
				ReasonIds::ID_TRADE_REQUEST, req.ID, trade_id.get(), repeat_count);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true);
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

}
