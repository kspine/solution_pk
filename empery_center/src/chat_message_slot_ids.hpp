#ifndef EMPERY_CENTER_CHAT_MESSAGE_SLOT_IDS_HPP_
#define EMPERY_CENTER_CHAT_MESSAGE_SLOT_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ChatMessageSlotIds {

constexpr ChatMessageSlotId
	ID_TEXT                 (     1 ),
	ID_SMILEY               (     2 ),
	ID_VOICE                (     3 ),

	ID_TAXER                ( 86201 ),
	ID_TAX_AMOUNT           ( 86202 ),

	ID_AUCTION_ITEM_ID      ( 86301 ),
	ID_AUCTION_ITEM_COUNT   ( 86302 );

}

}

#endif
