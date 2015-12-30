#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_item.hpp"
#include "../msg/sc_item.hpp"
#include "../msg/err_item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../data/item.hpp"
#include "../transaction_element.hpp"
#include "../item_ids.hpp"
#include "../reason_ids.hpp"
#include "../data/global.hpp"

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
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction);
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
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction);
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
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction);
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
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
	const auto items_consumed = checked_mul(repeat_count, unit_price);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, items_consumed,
		ReasonIds::ID_BUY_ACCELERATION_CARD, account->get_promotion_level(), 0, 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, { }, true); // 不计算税收。
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

}
