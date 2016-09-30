#ifndef EMPERY_CENTER_CHAT_MESSAGE_SLOT_IDS_HPP_
#define EMPERY_CENTER_CHAT_MESSAGE_SLOT_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace ChatMessageSlotIds {

constexpr ChatMessageSlotId
	ID_TEXT                         (     1 ),
	ID_SMILEY                       (     2 ),
	ID_VOICE                        (     3 ),

	ID_TAXER                        ( 86201 ),
	ID_TAX_AMOUNT                   ( 86202 ),

	ID_ITEM_ID                      ( 86301 ),
	ID_ITEM_COUNT                   ( 86302 ),
	ID_AUCTION_ITEM_BOX_ID          ( 86303 ),
	ID_AUCTION_ITEM_BOX_COUNT       ( 86304 ),

	ID_NEW_CASTLE_OWNER             ( 86401 ),
	ID_NEW_CASTLE_COORD_X           ( 86402 ),
	ID_NEW_CASTLE_COORD_Y           ( 86403 ),

	ID_HUP_REGAINED_ITEM_ID         ( 86411 ),
	ID_HUP_REGAINED_ITEM_COUNT      ( 86412 ),
	ID_HUP_DESTROYED_DEFENSE_ID     ( 86413 ),
	ID_HUP_DESTROYED_DEFENSE_LEVEL  ( 86414 ),
	ID_HUP_LAST_COORD_X             ( 86415 ),
	ID_HUP_LAST_COORD_Y             ( 86416 ),
	ID_HUP_CASTLE_NAME              ( 86417 ),

	ID_LEGEION_NAME                 ( 86421 ),
	ID_LEGEION_TASK_ID              ( 86422 ),
	ID_LEGION_STAGE                 ( 86423 ),
	ID_LEGION_PERSONAL_DONATE_ITEM  ( 86424 ),
	ID_LEGION_PERSONAL_DONATE_COUNT ( 86425 ),
	ID_LEGION_DONATE_ITMEM          ( 86426 ),
	ID_LEGION_DONATE_COUNT          ( 86427 );
}

}

#endif
