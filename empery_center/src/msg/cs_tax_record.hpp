#ifndef EMPERY_CENTER_MSG_CS_TAX_RECORD_HPP_
#define EMPERY_CENTER_MSG_CS_TAX_RECORD_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_TaxRecordGetPagedRecords
#define MESSAGE_ID      1100
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (begin)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
