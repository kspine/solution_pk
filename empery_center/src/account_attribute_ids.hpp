#ifndef EMPERY_CENTER_ACCOUNT_ATTRIBUTE_IDS_HPP_
#define EMPERY_CENTER_ACCOUNT_ATTRIBUTE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace AccountAttributeIds {

constexpr AccountAttributeId
	ID_BEGIN                          (   0 ),
	ID_CUSTOM_PUBLIC_END              ( 100 ),
	ID_CUSTOM_END                     ( 200 ),
	ID_PUBLIC_END                     ( 300 ),
	ID_END                            ( 500 ),

	ID_GENDER                         (   1 ),

	ID_LAST_LOGGED_IN_TIME            ( 200 ),
	ID_LAST_LOGGED_OUT_TIME           ( 201 ),
	ID_AVATAR                         ( 202 ),

	ID_LAST_SIGNED_IN_TIME            ( 302 ),
	ID_SEQUENTIAL_SIGNED_IN_DAYS      ( 303 ),
	ID_AUCTION_CENTER_ENABLED         ( 304 ),

	ID_LAST_CHAT_TIME_ADJACENT        ( 400 ),
	ID_LAST_CHAT_TIME_TRADE           ( 401 ),
	ID_LAST_CHAT_TIME_ALLIANCE        ( 402 ),
	ID_GOLD_PAYMENT_ENABLED           ( 403 ),

	ID_VERIF_CODE                     ( 500 ),
	ID_VERIF_CODE_EXPIRY_TIME         ( 501 ),
	ID_VERIF_CODE_COOLDOWN            ( 502 ),
	ID_CASTLE_HARVESTED_COOLDOWN      ( 503 ),
	ID_LOGIN_TOKEN                    ( 504 ),
	ID_LOGIN_TOKEN_EXPIRY_TIME        ( 505 ),
	ID_SAVED_THIRD_TOKEN              ( 506 ),
	ID_FIRST_CASTLE_NAME_SET          ( 507 ),
	ID_NOVICIATE_PROTECTION_EXPIRED   ( 508 );

}

}

#endif
