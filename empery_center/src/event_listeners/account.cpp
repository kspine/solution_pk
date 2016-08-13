#include "../precompiled.hpp"
#include "common.hpp"
#include "../events/account.hpp"
#include "../msg/sl_account.hpp"
#include "../singletons/log_client.hpp"

namespace EmperyCenter {

EVENT_LISTENER(Events::AccountCreated, event){
	const auto log = LogClient::require();

	Msg::SL_AccountCreated msg;
	msg.account_uuid = event.account_uuid.str();
	msg.remote_ip    = event.remote_ip;
	log->send(msg);
}

EVENT_LISTENER(Events::AccountLoggedIn, event){
	const auto log = LogClient::require();

	Msg::SL_AccountLoggedIn msg;
	msg.account_uuid = event.account_uuid.str();
	msg.remote_ip    = event.remote_ip;
	log->send(msg);
}

EVENT_LISTENER(Events::AccountLoggedOut, event){
	const auto log = LogClient::require();

	Msg::SL_AccountLoggedOut msg;
	msg.account_uuid    = event.account_uuid.str();
	msg.online_duration = event.online_duration;
	log->send(msg);
}

EVENT_LISTENER(Events::AccountNumberOnline, event){
	const auto log = LogClient::require();

	Msg::SL_AccountNumberOnline msg;
	msg.interval      = event.interval;
	msg.account_count = event.account_count;
	log->send(msg);
}

}
