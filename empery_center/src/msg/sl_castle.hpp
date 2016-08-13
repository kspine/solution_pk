#ifndef EMPERY_CENTER_MSG_SL_CASTLE_HPP_
#define EMPERY_CENTER_MSG_SL_CASTLE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SL_CastleResourceChanged
#define MESSAGE_ID      58301
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_VUINT         (resource_id)	\
	FIELD_VUINT         (old_amount)	\
	FIELD_VUINT         (new_amount)	\
	FIELD_VUINT         (reason)	\
	FIELD_VINT          (param1)	\
	FIELD_VINT          (param2)	\
	FIELD_VINT          (param3)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_CastleSoldierChanged
#define MESSAGE_ID      58302
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VUINT         (old_count)	\
	FIELD_VUINT         (new_count)	\
	FIELD_VUINT         (reason)	\
	FIELD_VINT          (param1)	\
	FIELD_VINT          (param2)	\
	FIELD_VINT          (param3)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_CastleWoundedSoldierChanged
#define MESSAGE_ID      58303
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VUINT         (old_count)	\
	FIELD_VUINT         (new_count)	\
	FIELD_VUINT         (reason)	\
	FIELD_VINT          (param1)	\
	FIELD_VINT          (param2)	\
	FIELD_VINT          (param3)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_CastleProtection
#define MESSAGE_ID      58304
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_VUINT         (delta_preparation_duration)	\
	FIELD_VUINT         (delta_protection_duration)	\
	FIELD_VUINT         (protection_end)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
