#ifndef EMPERY_CENTER_MSG_KS_MAP_HPP_
#define EMPERY_CENTER_MSG_KS_MAP_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    KS_MapRegisterCluster
#define MESSAGE_ID      32300
#define MESSAGE_FIELDS  \
	FIELD_VINT          (numerical_x)	\
	FIELD_VINT          (numerical_y)	\
	FIELD_STRING        (name)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    KS_MapUpdateMapObject
#define MESSAGE_ID      32301
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT         (attribute_id)	\
		FIELD_VINT          (value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    KS_MapRemoveMapObject
#define MESSAGE_ID      32302
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    KS_MapHarvestOverlay
#define MESSAGE_ID      32303
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (interval)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    KS_MapDeployImmigrants
#define MESSAGE_ID      32304
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_STRING        (castle_name)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    KS_MapEnterCastle
#define MESSAGE_ID      32305
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_STRING        (castle_uuid)
#include <poseidon/cbpp/message_generator.hpp>

// 32306

#define MESSAGE_NAME    KS_MapHarvestStrategicResource
#define MESSAGE_ID      32307
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (interval)
#include <poseidon/cbpp/message_generator.hpp>

// 32308

}

}

#endif
