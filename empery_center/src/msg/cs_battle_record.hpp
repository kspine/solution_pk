#ifndef EMPERY_CENTER_MSG_CS_BATTLE_RECORD_HPP_
#define EMPERY_CENTER_MSG_CS_BATTLE_RECORD_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_BattleRecordGetPagedRecords
#define MESSAGE_ID      1300
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (begin)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_BattleRecordGetPagedCrateRecords
#define MESSAGE_ID      1301
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (begin)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
