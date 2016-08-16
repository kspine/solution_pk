#include "../precompiled.hpp"
#include "common.hpp"
#include "../events/castle.hpp"
#include "../msg/sl_castle.hpp"
#include "../singletons/log_client.hpp"

namespace EmperyCenter {

EVENT_LISTENER(Events::CastleResourceChanged, event){
	const auto log = LogClient::require();

	Msg::SL_CastleResourceChanged msg;
	msg.map_object_uuid = event.map_object_uuid.str();
	msg.owner_uuid      = event.owner_uuid.str();
	msg.resource_id     = event.resource_id.get();
	msg.old_amount      = event.old_amount;
	msg.new_amount      = event.new_amount;
	msg.reason          = event.reason.get();
	msg.param1          = event.param1;
	msg.param2          = event.param2;
	msg.param3          = event.param3;
	log->send(msg);
}

EVENT_LISTENER(Events::CastleSoldierChanged, event){
	const auto log = LogClient::require();

	Msg::SL_CastleSoldierChanged msg;
	msg.map_object_uuid    = event.map_object_uuid.str();
	msg.owner_uuid         = event.owner_uuid.str();
	msg.map_object_type_id = event.map_object_type_id.get();
	msg.old_count          = event.old_count;
	msg.new_count          = event.new_count;
	msg.reason             = event.reason.get();
	msg.param1             = event.param1;
	msg.param2             = event.param2;
	msg.param3             = event.param3;
	log->send(msg);
}

EVENT_LISTENER(Events::CastleWoundedSoldierChanged, event){
	const auto log = LogClient::require();

	Msg::SL_CastleWoundedSoldierChanged msg;
	msg.map_object_uuid    = event.map_object_uuid.str();
	msg.owner_uuid         = event.owner_uuid.str();
	msg.map_object_type_id = event.map_object_type_id.get();
	msg.old_count          = event.old_count;
	msg.new_count          = event.new_count;
	msg.reason             = event.reason.get();
	msg.param1             = event.param1;
	msg.param2             = event.param2;
	msg.param3             = event.param3;
	log->send(msg);
}

EVENT_LISTENER(Events::CastleProtection, event){
	const auto log = LogClient::require();

	Msg::SL_CastleProtection msg;
	msg.map_object_uuid            = event.map_object_uuid.str();
	msg.owner_uuid                 = event.owner_uuid.str();
	msg.delta_preparation_duration = event.delta_preparation_duration;
	msg.delta_protection_duration  = event.delta_protection_duration;
	msg.protection_end             = event.protection_end;
	log->send(msg);
}

}
