#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_item.hpp"
#include "../msg/sc_item.hpp"
#include "../msg/err_item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../data/item.hpp"
#include "../transaction_element.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_ItemGetAllItems, account_uuid, session, /* req */){
	const auto item_box = ItemBoxMap::require(account_uuid);
	item_box->pump_status();
	item_box->synchronize_with_player(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_ItemTradeFromRecharge, account_uuid, session, req){
	const auto item_box = ItemBoxMap::require(account_uuid);
	item_box->pump_status();

	const auto repeat_count = req.repeat_count;
	if(repeat_count == 0){
		return Response(Msg::ERR_ZERO_REPEAT_TIMES);
	}
	const auto recharge_id = RechargeId(req.recharge_id);
	const auto recharge_data = Data::ItemRecharge::get(recharge_id);
	if(!recharge_data){
		return Response(Msg::ERR_NO_SUCH_RECHARGE_ID) <<recharge_id;
	}
	const auto trade_data = Data::ItemTrade::require(recharge_data->trade_id);

	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, repeat_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction.data(), transaction.size());
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_ItemTradeFromShop, account_uuid, session, req){
	const auto item_box = ItemBoxMap::require(account_uuid);
	item_box->pump_status();

	const auto repeat_count = req.repeat_count;
	if(repeat_count == 0){
		return Response(Msg::ERR_ZERO_REPEAT_TIMES);
	}
	const auto shop_id = ShopId(req.shop_id);
	const auto shop_data = Data::ItemShop::get(shop_id);
	if(!shop_data){
		return Response(Msg::ERR_NO_SUCH_SHOP_ID) <<shop_id;
	}
	const auto trade_data = Data::ItemTrade::require(shop_data->trade_id);

	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, repeat_count, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction.data(), transaction.size());
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

}
