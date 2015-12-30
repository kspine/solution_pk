#ifndef EMPERY_CENTER_MSG_CS_ITEM_HPP_
#define EMPERY_CENTER_MSG_CS_ITEM_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_ItemGetAllItems
#define MESSAGE_ID      500
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ItemTradeFromRecharge
#define MESSAGE_ID      501
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (recharge_id)	\
	FIELD_VUINT         (repeat_count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ItemTradeFromShop
#define MESSAGE_ID      502
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (shop_id)	\
	FIELD_VUINT         (repeat_count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ItemUseItem
#define MESSAGE_ID      503
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (repeat_count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ItemBuyAccelerationCards
#define MESSAGE_ID      504
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (use_gold)	\
	FIELD_VUINT         (repeat_count)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
