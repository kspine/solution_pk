#ifndef EMPERY_CENTER_MSG_CS_MAP_HPP_
#define EMPERY_CENTER_MSG_CS_MAP_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME	CS_MapQueryWorldMap
#define MESSAGE_ID		300
#define MESSAGE_FIELDS	\
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	CS_MapSetView
#define MESSAGE_ID		301
#define MESSAGE_FIELDS	\
	FIELD_VINT			(x)	\
	FIELD_VINT			(y)	\
	FIELD_VUINT			(width)	\
	FIELD_VUINT			(height)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	CS_MapRefreshView
#define MESSAGE_ID		302
#define MESSAGE_FIELDS	\
	FIELD_VINT			(x)	\
	FIELD_VINT			(y)	\
	FIELD_VUINT			(width)	\
	FIELD_VUINT			(height)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	CS_MapSetObjectPath
#define MESSAGE_ID		303
#define MESSAGE_FIELDS	\
	FIELD_STRING		(mapObjectUuid)	\
	FIELD_VINT			(x)	\
	FIELD_VINT			(y)	\
	FIELD_ARRAY			(waypoints,	\
		FIELD_VINT			(dx)	\
		FIELD_VINT			(dy)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
