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
	FIELD_VINT          (y)	\
	FIELD_VUINT         (protection_cost_notified)
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

#define MESSAGE_NAME    CS_MapJumpToAnotherCluster
#define MESSAGE_ID      309
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (direction)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapDismissBattalion
#define MESSAGE_ID      310
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapEvictBattalionFromCastle
#define MESSAGE_ID      311
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapRefillBattalion
#define MESSAGE_ID      312
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (soldier_count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapLoadMinimap
#define MESSAGE_ID      313
#define MESSAGE_FIELDS  \
	FIELD_VINT          (left)	\
	FIELD_VINT          (bottom)	\
	FIELD_VUINT         (width)	\
	FIELD_VUINT         (height)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapHarvestMapCell
#define MESSAGE_ID      314
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapCreateDefenseBuilding
#define MESSAGE_ID      315
#define MESSAGE_FIELDS  \
	FIELD_STRING        (castle_uuid)	\
	FIELD_VUINT         (map_object_type_id)	\
	FIELD_VINT          (coord_x)	\
	FIELD_VINT          (coord_y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapUpgradeDefenseBuilding
#define MESSAGE_ID      316
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapDestroyDefenseBuilding
#define MESSAGE_ID      317
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapCompleteDefenseBuildingImmediately
#define MESSAGE_ID      318
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapSpeedUpDefenseBuildingUpgrade
#define MESSAGE_ID      319
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapReturnOccupiedMapCell
#define MESSAGE_ID      320
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapGarrisonBattleBunker
#define MESSAGE_ID      321
#define MESSAGE_FIELDS  \
	FIELD_STRING        (battalion_uuid)	\
	FIELD_STRING        (bunker_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapEvictBattleBunker
#define MESSAGE_ID      322
#define MESSAGE_FIELDS  \
	FIELD_STRING        (bunker_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapRefreshMapObject
#define MESSAGE_ID      323
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapLayoffsBattalion
#define MESSAGE_ID      324
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_VUINT         (soldier_count)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
