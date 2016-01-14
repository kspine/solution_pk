#ifndef EMPERY_CENTER_MSG_CS_AUCTION_HPP_
#define EMPERY_CENTER_MSG_CS_AUCTION_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_AuctionQueryStatus
#define MESSAGE_ID      1000
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_AuctionUpdateTransferStatus
#define MESSAGE_ID      1001
#define MESSAGE_FIELDS  \
	FIELD_STRING        (map_object_uuid)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
