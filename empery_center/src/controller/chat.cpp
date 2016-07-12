#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/ts_chat.hpp"
#include "../msg/err_chat.hpp"
#include "../singletons/chat_box_map.hpp"
#include "../horn_message.hpp"
#include "../singletons/player_session_map.hpp"
#include "../player_session.hpp"

namespace EmperyCenter {

CONTROLLER_SERVLET(Msg::TS_ChatBroadcastHornMessage, controller, req){
	const auto horn_message_uuid = HornMessageUuid(req.horn_message_uuid);
	const auto horn_message = ChatBoxMap::forced_reload_horn_message(horn_message_uuid);
	if(!horn_message){
		return Response(Msg::ERR_NO_SUCH_HORN_MESSAGE) <<horn_message_uuid;
	}

	std::vector<std::pair<boost::shared_ptr<Account>, boost::shared_ptr<PlayerSession>>> sessions;
	PlayerSessionMap::get_all(sessions);
	for(auto it = sessions.begin(); it != sessions.end(); ++it){
		const auto &session = it->second;
		try {
			horn_message->synchronize_with_player(session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return Response();
}

}
