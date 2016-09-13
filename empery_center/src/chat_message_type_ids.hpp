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

	ID_LEVEL_LEGION_JOIN            ( 103 ),
	ID_LEVEL_LEGION_REFUSE_INVITE   ( 104 ),
	ID_LEVEL_LEGION_DISBAND         ( 105 ),
	ID_LEVEL_LEGION_KICK            ( 106 ),
	ID_LEVEL_LEGION_REFUSE_APPLY    ( 107 ),

	ID_LEVEL_LEAGUE_JOIN            ( 130 ),
	ID_LEVEL_LEAGUE_REFUSE_INVITE   ( 131 ),
	ID_LEVEL_LEAGUE_DISBAND         ( 132 ),
	ID_LEVEL_LEAGUE_KICK            ( 133 ),
	ID_LEVEL_LEAGUE_REFUSE_APPLY    ( 134 ),

	ID_AUCTION_RESULT               ( 200 ),
	ID_PAYMENT_DIAMONDS             ( 201 ),
	ID_PAYMENT_GOLD_COINS           ( 202 ),
	ID_PAYMENT_GIFT_BOXES           ( 203 ),

	ID_CASTLE_HUNG_UP               ( 300 ),
	ID_NOVICIATE_PROTECTION_FINISH  ( 301 ),
	ID_LEGION_TASK_REWARD           ( 302 );

}

}

#endif
