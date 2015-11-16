#ifndef EMPERY_CENTER_MSG_SC_MAP_HPP_
#define EMPERY_CENTER_MSG_SC_MAP_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_MapWorldMapList
#define MESSAGE_ID      399
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (maps,	\
		FIELD_VINT          (map_x)	\
		FIELD_VINT          (map_y)	\
	)	\
	FIELD_VUINT         (map_width)	\
	FIELD_VUINT         (map_height)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapCellInfo
#define MESSAGE_ID      398
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (terrain_id)	\
	FIELD_VUINT         (overlay_id)	\
	FIELD_VUINT         (resource_id)	\
	FIELD_VUINT         (resource_count)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_ARRAY         (buff_set,	\
		FIELD_VUINT         (buff_id)	\
		FIELD_STRING        (buff_owner_uuid)	\
		FIELD_VINT          (buff_value)	\
		FIELD_VUINT         (expiry_duration)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapObjectInfo
#define MESSAGE_ID      397
#define MESSAGE_FIELDS  \
	FIELD_STRING        (object_uuid)	\
	FIELD_VUINT         (object_type_id)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_STRING        (name)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT         (slot)	\
		FIELD_VINT          (value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapObjectRemoved
#define MESSAGE_ID      396
#define MESSAGE_FIELDS  \
	FIELD_STRING        (object_uuid)	\
	FIELD_VUINT         (reason)	\
	FIELD_STRING        (param)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
