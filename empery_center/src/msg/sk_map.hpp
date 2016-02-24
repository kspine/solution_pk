#ifndef EMPERY_CENTER_MSG_SK_MAP_HPP_
#define EMPERY_CENTER_MSG_SK_MAP_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SK_MapClusterRegistrationSucceeded
#define MESSAGE_ID      32399
#define MESSAGE_FIELDS  \
	FIELD_VINT          (cluster_x)	\
	FIELD_VINT          (cluster_y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SK_MapAddMapCell
#define MESSAGE_ID      32398
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_STRING        (parent_object_uuid)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT         (attribute_id)	\
		FIELD_VINT          (value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SK_MapAddMapObject
#define MESSAGE_ID      32397
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_STRING        (parent_object_uuid)	\
	FIELD_VUINT         (garrisoned)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT         (attribute_id)	\
		FIELD_VINT          (value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SK_MapRemoveMapObject
#define MESSAGE_ID      32396
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SK_MapSetAction
#define MESSAGE_ID      32395
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_ARRAY         (waypoints,	\
		FIELD_VUINT         (delay)	\
		FIELD_VINT          (dx)	\
		FIELD_VINT          (dy)	\
	)	\
	FIELD_VUINT         (action)	\
	FIELD_STRING        (param)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SK_MapAddOverlay
#define MESSAGE_ID      32394
#define MESSAGE_FIELDS  \
	FIELD_VINT          (cluster_x)	\
	FIELD_VINT          (cluster_y)	\
	FIELD_STRING        (overlay_group_name)	\
	FIELD_VUINT         (overlay_id)	\
	FIELD_VUINT         (resource_id)	\
	FIELD_VUINT         (resource_amount)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
