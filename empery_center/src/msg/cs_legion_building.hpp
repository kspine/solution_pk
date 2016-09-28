#ifndef EMPERY_CENTER_MSG_CS_LEGION_HPP_
#define EMPERY_CENTER_MSG_CS_LEGION_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_GetLegionBuildingInfoMessage
#define MESSAGE_ID      1630
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CreateLegionBuildingMessage
#define MESSAGE_ID      1631
#define MESSAGE_FIELDS  \
	FIELD_STRING        (castle_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VINT          (coord_x)	\
	FIELD_VINT          (coord_y)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_UpgradeLegionBuildingMessage
#define MESSAGE_ID      1632
#define MESSAGE_FIELDS  \
	FIELD_STRING          (castle_uuid)	\
	FIELD_STRING          (legion_building_uuid)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_CancleUpgradeLegionBuildingMessage
#define MESSAGE_ID      1633
#define MESSAGE_FIELDS  \
	FIELD_STRING          (legion_building_uuid)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_OpenLegionGrubeMessage
#define MESSAGE_ID      1634
#define MESSAGE_FIELDS  \
	FIELD_STRING          (legion_building_uuid) \
	FIELD_VUINT           (ntype) \
	FIELD_VUINT           (consume)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_RepairLegionGrubeMessage
#define MESSAGE_ID      1635
#define MESSAGE_FIELDS  \
	FIELD_STRING          (legion_building_uuid) \
	FIELD_STRING          (type) \
	FIELD_VUINT           (percent)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_GradeLegionMessage
#define MESSAGE_ID      1636
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_DemolishLegionGrubeMessage
#define MESSAGE_ID      1638
#define MESSAGE_FIELDS  \
	FIELD_STRING          (legion_building_uuid)

#include <poseidon/cbpp/message_generator.hpp>
}



}

#endif
