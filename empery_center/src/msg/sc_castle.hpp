#ifndef EMPERY_CENTER_MSG_SC_CASTLE_HPP_
#define EMPERY_CENTER_MSG_SC_CASTLE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME	SC_CastleBuildingBase
#define MESSAGE_ID		499
#define MESSAGE_FIELDS	\
	FIELD_STRING		(mapObjectUuid)	\
	FIELD_VUINT			(buildingBaseId)	\
	FIELD_VUINT			(buildingId)	\
	FIELD_VUINT			(buildingLevel)	\
	FIELD_VUINT			(mission)	\
	FIELD_VUINT			(missionParam1)	\
	FIELD_VUINT			(missionParam2)	\
	FIELD_VUINT			(missionTimeBegin)	\
	FIELD_VUINT			(missionTimeRemaining)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	SC_CastleResource
#define MESSAGE_ID		498
#define MESSAGE_FIELDS	\
	FIELD_STRING		(mapObjectUuid)	\
	FIELD_VUINT			(resourceId)	\
	FIELD_VUINT			(count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	SC_CastleTech
#define MESSAGE_ID		497
#define MESSAGE_FIELDS	\
	FIELD_STRING		(mapObjectUuid)	\
	FIELD_VUINT			(techId)	\
	FIELD_VUINT			(techLevel)	\
	FIELD_VUINT			(mission)	\
	FIELD_VUINT			(missionParam1)	\
	FIELD_VUINT			(missionParam2)	\
	FIELD_VUINT			(missionTimeBegin)	\
	FIELD_VUINT			(missionTimeRemaining)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
