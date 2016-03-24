#ifndef EMPERY_CENTER_MSG_SC_BATTLE_RECORD_HPP_
#define EMPERY_CENTER_MSG_SC_BATTLE_RECORD_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_BattleRecordPagedRecords
#define MESSAGE_ID      1399
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (total_count)	\
	FIELD_VUINT         (begin)	\
	FIELD_ARRAY         (records,	\
		FIELD_VUINT         (timestamp)	\
		FIELD_VUINT         (first_object_type_id)	\
		FIELD_VINT          (first_coord_x)	\
		FIELD_VINT          (first_coord_y)	\
		FIELD_STRING        (second_account_uuid)	\
		FIELD_VUINT         (second_object_type_id)	\
		FIELD_VINT          (second_coord_x)	\
		FIELD_VINT          (second_coord_y)	\
		FIELD_VINT          (result_type)	\
		FIELD_VINT          (result_param1)	\
		FIELD_VINT          (result_param2)	\
		FIELD_VUINT         (soldiers_damaged)	\
		FIELD_VUINT         (soldiers_remaining)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_BattleRecordReceivedNew
#define MESSAGE_ID      1398
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (reserved)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
