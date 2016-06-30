#ifndef EMPERY_CENTER_CHAT_MESSAGE_TYPE_IDS_HPP_
#define EMPERY_CENTER_CHAT_MESSAGE_TYPE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ChatMessageTypeIds {

constexpr ChatMessageTypeId
	ID_PLAIN                        (   1 ),
	ID_NEW_CASTLE_NOTIFICATION      (   2 ),

	ID_LEVEL_BONUS                  ( 100 ),
	ID_INCOME_TAX                   ( 101 ),
	ID_LEVEL_BONUS_EXTRA            ( 102 ),

	ID_AUCTION_RESULT               ( 200 ),
	ID_PAYMENT_DIAMONDS             ( 201 ),
	ID_PAYMENT_GOLD_COINS           ( 202 ),
	ID_PAYMENT_GIFT_BOXES           ( 203 ),

	ID_CASTLE_HUNG_UP               ( 300 ),
	ID_NOVICIATE_PROTECTION_FINISH  ( 301 );

}

}

#endif
