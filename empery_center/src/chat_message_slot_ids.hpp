#ifndef EMPERY_CENTER_CHAT_MESSAGE_SLOT_IDS_HPP_
#define EMPERY_CENTER_CHAT_MESSAGE_SLOT_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ChatMessageSlotIds {

constexpr ChatMessageSlotId
	ID_TEXT                     (     1 ),
	ID_SMILEY                   (     2 ),
	ID_VOICE                    (     3 ),

	ID_TAXER                    ( 86201 ),
	ID_TAX_AMOUNT               ( 86202 ),

	ID_ITEM_ID                  ( 86301 ),
	ID_ITEM_COUNT               ( 86302 ),
	ID_AUCTION_ITEM_BOX_ID      ( 86303 ),
	ID_AUCTION_ITEM_BOX_COUNT   ( 86304 ),

	ID_IMMIGRANT_OWNER          ( 86401 ),
	ID_IMMIGRANT_COORD_X        ( 86402 ),
	ID_IMMIGRANT_COORD_Y        ( 86403 );

}

}

#endif
