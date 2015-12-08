#ifndef EMPERY_CENTER_ACCOUNT_ATTRIBUTE_IDS_HPP_
#define EMPERY_CENTER_ACCOUNT_ATTRIBUTE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

namespace AccountAttributeIds {

constexpr AccountAttributeId
	ID_BEGIN                      (   0),
	ID_CUSTOM_PUBLIC_END          ( 100),
	ID_CUSTOM_END                 ( 200),
	ID_PUBLIC_END                 ( 300),
	ID_END                        ( 500),

	ID_GENDER                     (   1),
	ID_AVATAR                     (   2),

	ID_LAST_LOGGED_IN_TIME        ( 300),
	ID_LAST_LOGGED_OUT_TIME       ( 301),
	ID_LAST_SIGNED_IN_TIME        ( 302),
	ID_SEQUENTIAL_SIGNED_IN_DAYS  ( 303);

}

}

#endif
