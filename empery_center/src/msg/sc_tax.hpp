#ifndef EMPERY_CENTER_MSG_SC_TAX_HPP_
#define EMPERY_CENTER_MSG_SC_TAX_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_TaxRecordsPaged
#define MESSAGE_ID      1199
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (total_count)	\
	FIELD_VUINT         (begin)	\
	FIELD_ARRAY         (records,	\
		FIELD_VUINT         (reserved)	\
		FIELD_VUINT         (timestamp)	\
		FIELD_STRING        (from_account_uuid)	\
		FIELD_VUINT         (reason)	\
		FIELD_VUINT         (old_amount)	\
		FIELD_VUINT         (new_amount)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_TaxNewRecord
#define MESSAGE_ID      1199
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (reserved)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
