#ifndef EMPERY_CENTER_MSG_SC_MAP_HPP_
#define EMPERY_CENTER_MSG_SC_MAP_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_MapWorldMapList
#define MESSAGE_ID      399
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (clusters,	\
		FIELD_VINT          (x)	\
		FIELD_VINT          (y)	\
		FIELD_STRING        (name)	\
	)	\
	FIELD_VUINT         (cluster_width)	\
	FIELD_VUINT         (cluster_height)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapCellInfo
#define MESSAGE_ID      398
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_STRING        (parent_object_uuid)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_VUINT         (acceleration_card_applied)	\
	FIELD_VUINT         (ticket_item_id)	\
	FIELD_VUINT         (production_resource_id)	\
	FIELD_VUINT         (resource_amount)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT         (attribute_id)	\
		FIELD_VINT          (value)	\
	)	\
	FIELD_VUINT         (production_rate)	\
	FIELD_VUINT         (capacity)	\
	FIELD_VINT          (parent_object_x)	\
	FIELD_VINT          (parent_object_y)	\
	FIELD_ARRAY         (buffs,	\
		FIELD_VUINT         (buff_id)	\
		FIELD_VUINT         (duration)	\
		FIELD_VUINT         (time_remaining)	\
	)	\
	FIELD_STRING        (occupier_object_uuid)	\
	FIELD_STRING        (occupier_owner_uuid)	\
	FIELD_VINT          (occupier_x)	\
	FIELD_VINT          (occupier_y)	\
	FIELD_STRING        (occupier_name)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapObjectInfo
#define MESSAGE_ID      397
#define MESSAGE_FIELDS  \
	FIELD_STRING        (object_uuid)	\
	FIELD_VUINT         (object_type_id)	\
	FIELD_STRING        (owner_uuid)	\
	FIELD_STRING        (parent_object_uuid)	\
	FIELD_STRING        (name)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_ARRAY         (attributes,	\
		FIELD_VUINT         (attribute_id)	\
		FIELD_VINT          (value)	\
	)	\
	FIELD_VUINT         (garrisoned)	\
	FIELD_VUINT         (action)    \
	FIELD_STRING        (param)	\
	FIELD_ARRAY         (buffs,	\
		FIELD_VUINT         (buff_id)	\
		FIELD_VUINT         (duration)	\
		FIELD_VUINT         (time_remaining)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapObjectRemoved
#define MESSAGE_ID      396
#define MESSAGE_FIELDS  \
	FIELD_STRING        (object_uuid)	\
	FIELD_VUINT         (object_type_id)	\
	FIELD_VUINT         (reserved)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapCellRemoved
#define MESSAGE_ID      395
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapStopTroopsRet
#define MESSAGE_ID      394
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (map_objects,   \
		FIELD_STRING        (map_object_uuid)   \
	)
#include <poseidon/cbpp/message_generator.hpp>
/*
#define MESSAGE_NAME    SC_MapOverlayInfo
#define MESSAGE_ID      393
#define MESSAGE_FIELDS  \
	FIELD_VINT          (cluster_x)	\
	FIELD_VINT          (cluster_y)	\
	FIELD_STRING        (overlay_group_name)	\
	FIELD_VUINT         (resource_amount)	\
	FIELD_STRING        (last_harvested_account_uuid)	\
	FIELD_STRING        (last_harvested_object_uuid)	\
	FIELD_VINT          (coord_hint_x)	\
	FIELD_VINT          (coord_hint_y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapOverlayRemoved
#define MESSAGE_ID      392
#define MESSAGE_FIELDS  \
	FIELD_VINT          (cluster_x)	\
	FIELD_VINT          (cluster_y)	\
	FIELD_STRING        (overlay_group_name)	\
	FIELD_VINT          (coord_hint_x)	\
	FIELD_VINT          (coord_hint_y)
#include <poseidon/cbpp/message_generator.hpp>
*/
#define MESSAGE_NAME    SC_MapObjectStopped
#define MESSAGE_ID      391
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (action)	\
	FIELD_STRING        (param)	\
	FIELD_VINT          (error_code)	\
	FIELD_STRING        (error_message)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapObjectAttackResult
#define MESSAGE_ID      390
#define MESSAGE_FIELDS  \
	FIELD_STRING        (attacking_object_uuid)	\
	FIELD_VINT          (attacking_coord_x)	\
	FIELD_VINT          (attacking_coord_y)	\
	FIELD_STRING        (attacked_object_uuid)	\
	FIELD_VINT          (attacked_coord_x)	\
	FIELD_VINT          (attacked_coord_y)	\
	FIELD_VINT          (result_type)	\
	FIELD_VUINT         (soldiers_wounded)	\
	FIELD_VUINT         (soldiers_wounded_added)	\
	FIELD_VUINT         (soldiers_damaged)	\
	FIELD_VUINT         (soldiers_remaining)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapMinimap
#define MESSAGE_ID      389
#define MESSAGE_FIELDS  \
	FIELD_VINT          (left)	\
	FIELD_VINT          (bottom)	\
	FIELD_VUINT         (width)	\
	FIELD_VUINT         (height)	\
	FIELD_ARRAY         (castles,	\
		FIELD_VINT          (x)	\
		FIELD_VINT          (y)	\
		FIELD_STRING        (owner_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapStrategicResourceInfo
#define MESSAGE_ID      388
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (resource_id)	\
	FIELD_VUINT         (resource_amount)	\
	FIELD_STRING        (last_harvested_account_uuid)	\
	FIELD_STRING        (last_harvested_object_uuid)	\
	FIELD_VUINT         (map_event_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapStrategicResourceRemoved
#define MESSAGE_ID      387
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)
#include <poseidon/cbpp/message_generator.hpp>

// 386

#define MESSAGE_NAME    SC_MapWaypointsSet
#define MESSAGE_ID      385
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_ARRAY         (waypoints,	\
		FIELD_VINT          (dx)	\
		FIELD_VINT          (dy)	\
	)	\
	FIELD_VUINT         (action)	\
	FIELD_STRING        (param)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapMonsterRewardGot
#define MESSAGE_ID      384
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_ARRAY         (items_basic,	\
		FIELD_VUINT         (item_id)	\
		FIELD_VUINT         (count)	\
	)	\
	FIELD_ARRAY         (items_extra,	\
		FIELD_VUINT         (item_id)	\
		FIELD_VUINT         (count)	\
	)	\
	FIELD_STRING        (castle_uuid)	\
	FIELD_VUINT         (reward_counter)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapResourceCrateInfo
#define MESSAGE_ID      383
#define MESSAGE_FIELDS  \
	FIELD_STRING        (resource_crate_uuid)	\
	FIELD_VUINT         (resource_id)	\
	FIELD_VUINT         (amount_max)	\
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (expiry_duration)	\
	FIELD_VUINT         (amount_remaining)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapResourceCrateRemoved
#define MESSAGE_ID      382
#define MESSAGE_FIELDS  \
	FIELD_STRING        (resource_crate_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapResourceCrateHarvestResult
#define MESSAGE_ID      381
#define MESSAGE_FIELDS  \
	FIELD_STRING        (attacking_object_uuid)	\
	FIELD_VINT          (attacking_coord_x)	\
	FIELD_VINT          (attacking_coord_y)	\
	FIELD_STRING        (resource_crate_uuid)	\
	FIELD_VINT          (attacked_coord_x)	\
	FIELD_VINT          (attacked_coord_y)	\
	FIELD_VUINT         (resource_id)	\
	FIELD_VUINT         (amount_harvested)	\
	FIELD_VUINT         (amount_remaining)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapDefenseBuilding
#define MESSAGE_ID      380
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (building_level)	\
	FIELD_VUINT         (mission)	\
	FIELD_VUINT         (mission_duration)	\
	FIELD_VUINT         (mission_time_remaining)	\
	FIELD_STRING        (garrisoning_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapObjectAttackMapCellResult
#define MESSAGE_ID      379
#define MESSAGE_FIELDS  \
	FIELD_STRING        (attacking_object_uuid)	\
	FIELD_VINT          (attacking_coord_x)	\
	FIELD_VINT          (attacking_coord_y)	\
	FIELD_VUINT         (attacked_ticket_item_id)	\
	FIELD_VINT          (attacked_coord_x)	\
	FIELD_VINT          (attacked_coord_y)	\
	FIELD_VUINT         (soldiers_damaged)	\
	FIELD_VUINT         (soldiers_remaining)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapResourceCrateExplosion
#define MESSAGE_ID      378
#define MESSAGE_FIELDS  \
	FIELD_VINT          (coord_x)	\
	FIELD_VINT          (coord_y)	\
	FIELD_VUINT         (map_object_type_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapCastleRelocation
#define MESSAGE_ID      377
#define MESSAGE_FIELDS  \
	FIELD_STRING        (castle_uuid)	\
	FIELD_VUINT         (level)	\
	FIELD_VINT          (old_coord_x)	\
	FIELD_VINT          (old_coord_y)	\
	FIELD_VINT          (new_coord_x)	\
	FIELD_VINT          (new_coord_y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapActivityAcculateReward
#define MESSAGE_ID      376
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VINT          (activity_id) \
	FIELD_ARRAY         (items_basic,	\
		FIELD_VUINT         (item_id)	\
		FIELD_VUINT         (count)	\
	)	\
	FIELD_ARRAY         (reward_acculate,	\
		FIELD_VUINT         (acculate)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_MapGoblinRewardGot
#define MESSAGE_ID      375
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_ARRAY         (items_basic,	\
		FIELD_VUINT         (item_id)	\
		FIELD_VUINT         (count)	\
	)	\
	FIELD_STRING        (castle_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
