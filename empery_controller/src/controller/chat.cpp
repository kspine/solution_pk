#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/st_chat.hpp"
#include "../../../empery_center/src/msg/ts_chat.hpp"
#include "../../../empery_center/src/msg/err_chat.hpp"
#include "../singletons/world_map.hpp"

namespace EmperyController {

CONTROLLER_SERVLET(Msg::ST_ChatInvalidateHornMessage, controller, req){
	const auto horn_message_uuid = HornMessageUuid(req.horn_message_uuid);

	Msg::TS_ChatInvalidateHornMessage msg;
	msg.horn_message_uuid = horn_message_uuid.str();

	std::vector<std::pair<Coord, boost::shared_ptr<ControllerSession>>> controllers;
	WorldMap::get_all_controllers(controllers);
	for(auto it = controllers.begin(); it != controllers.end(); ++it){
		const auto &other_controller = it->second;
		if(other_controller == controller){
			continue;
		}
		try {
			other_controller->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CONTROLLER_WARNING("std::exception thrown: what = ", e.what());
		}
	}

	return Response();
}

}
