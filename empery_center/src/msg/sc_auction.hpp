#ifndef EMPERY_CENTER_MSG_SC_AUCTION_HPP_
#define EMPERY_CENTER_MSG_SC_AUCTION_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_AuctionItemChanged
#define MESSAGE_ID      1099
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (item_id)	\
	FIELD_VUINT         (count)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_AuctionTransferStatus
#define MESSAGE_ID      1098
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)	\
	FIELD_ARRAY         (items,	\
		FIELD_VUINT         (item_id)	\
		FIELD_VUINT         (item_count)	\
		FIELD_VUINT         (created_time)	\
		FIELD_VUINT         (due_time)	\
		FIELD_VUINT         (remaining_duration)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
