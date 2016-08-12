#include "../precompiled.hpp"
#include "common.hpp"
#include "../events/item.hpp"
#include "../msg/sl_item.hpp"
#include "../singletons/log_client.hpp"

namespace EmperyCenter {

EVENT_LISTENER(Events::ItemChanged, event){
	const auto log = LogClient::require();

	Msg::SL_ItemChanged msg;
	msg.account_uuid = event.account_uuid.str();
	msg.item_id      = event.item_id.get();
	msg.old_count    = event.old_count;
	msg.new_count    = event.new_count;
	msg.reason       = event.reason.get();
	msg.param1       = event.param1;
	msg.param2       = event.param2;
	msg.param3       = event.param3;
	log->send(msg);
}

}
