#ifndef EMPERY_CENTER_MSG_CS_MAP_HPP_
#define EMPERY_CENTER_MSG_CS_MAP_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_MapQueryWorldMap
#define MESSAGE_ID      300
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapSetView
#define MESSAGE_ID      301
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (width)	\
	FIELD_VUINT         (height)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapRefreshView
#define MESSAGE_ID      302
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (width)	\
	FIELD_VUINT         (height)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapSetWaypoints
#define MESSAGE_ID      303
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_ARRAY         (waypoints,	\
		FIELD_VINT          (dx)	\
		FIELD_VINT          (dy)	\
	)	\
	FIELD_VUINT         (action)	\
	FIELD_STRING		(param)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapPurchaseMapCell
#define MESSAGE_ID      304
#define MESSAGE_FIELDS  \
	FIELD_STRING        (parent_object_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (resource_id)	\
	FIELD_VUINT         (ticket_item_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapUpgradeMapCell
#define MESSAGE_ID      305
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)
#include <poseidon/cbpp/message_generator.hpp>
/*
#define MESSAGE_NAME    CS_MapDeployImmigrants
#define MESSAGE_ID      306
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_STRING        (castle_name)
#include <poseidon/cbpp/message_generator.hpp>
*/
#define MESSAGE_NAME    CS_MapStopTroops
#define MESSAGE_ID      307
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (map_objects,	\
		FIELD_STRING        (map_object_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapApplyAccelerationCard
#define MESSAGE_ID      308
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
