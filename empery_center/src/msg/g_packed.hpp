#ifndef EMPERY_CENTER_MSG_G_PACKED_HPP_
#define EMPERY_CENTER_MSG_G_PACKED_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

// 中心到集群双向。
#define MESSAGE_NAME    G_PackedRequest
#define MESSAGE_ID      99
#define MESSAGE_FIELDS  \
	FIELD_STRING        (uuid)	\
	FIELD_VUINT         (message_id)	\
	FIELD_STRING        (payload)
#include <poseidon/cbpp/message_generator.hpp>

// 中心到集群双向。和 G_PackedRequest 构成“请求-应答”对。
#define MESSAGE_NAME    G_PackedResponse
#define MESSAGE_ID      98
#define MESSAGE_FIELDS  \
	FIELD_STRING        (uuid)	\
	FIELD_VINT          (code)	\
	FIELD_STRING        (message)
#include <poseidon/cbpp/message_generator.hpp>

// 集群到中心单向。只转发，不需要响应。
#define MESSAGE_NAME    G_PackedAccountNotification
#define MESSAGE_ID      97
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_VUINT         (message_id)	\
	FIELD_STRING        (payload)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    G_PackedRectangleNotification
#define MESSAGE_ID      96
#define MESSAGE_FIELDS  \
	FIELD_VINT          (x)	\
	FIELD_VINT          (y)	\
	FIELD_VUINT         (width)	\
	FIELD_VUINT         (height)	\
	FIELD_VUINT         (message_id)	\
	FIELD_STRING        (payload)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    G_PackedDungeonNotification
#define MESSAGE_ID      95
#define MESSAGE_FIELDS  \
	FIELD_STRING        (dungeon_uuid)	\
	FIELD_VUINT         (message_id)	\
	FIELD_STRING        (payload)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
