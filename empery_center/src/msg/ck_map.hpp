#ifndef EMPERY_CENTER_MSG_CK_MAP_HPP_
#define EMPERY_CENTER_MSG_CK_MAP_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CK_MapClusterRegistrationSucceeded
#define MESSAGE_ID      32399
#define MESSAGE_FIELDS  \
	FIELD_VINT          (cluster_x)	\
	FIELD_VINT          (cluster_y)	\
	FIELD_VUINT         (width)	\
	FIELD_VUINT         (height)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CK_MapAddMapObject
#define MESSAGE_ID      32398
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT         (attribute_id)	\
		FIELD_VINT          (value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CK_MapRemoveMapObject
#define MESSAGE_ID      32397
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CK_MapSetAction
#define MESSAGE_ID      32396
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_ARRAY         (waypoints,	\
		FIELD_VUINT         (delay)	\
		FIELD_VINT          (dx)	\
		FIELD_VINT          (dy)	\
	)	\
	FIELD_STRING        (attack_target_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CK_MapAddMapCell
#define MESSAGE_ID      32395
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT         (attribute_id)	\
		FIELD_VINT          (value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
