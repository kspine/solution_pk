#ifndef EMPERY_CENTER_CHAT_MESSAGE_TYPE_IDS_HPP_
#define EMPERY_CENTER_CHAT_MESSAGE_TYPE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ChatMessageTypeIds {

constexpr ChatMessageTypeId
	ID_PLAIN                    (   1 ),
	ID_IMMIGRANT_NOTIFICATION   (   2 ),

	ID_LEVEL_BONUS              ( 100 ),
	ID_INCOME_TAX               ( 101 ),
	ID_LEVEL_BONUS_EXTRA        ( 102 ),

	ID_AUCTION_RESULT           ( 200 ),
	ID_PAYMENT_DIAMONDS         ( 201 ),
	ID_PAYMENT_GOLD_COINS       ( 202 ),
	ID_PAYMENT_GIFT_BOXES       ( 203 );

}

}

#endif
