#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_chat.hpp"
#include "../msg/sc_chat.hpp"
#include "../msg/err_chat.hpp"
#include "../singletons/chat_box_map.hpp"
#include "../chat_box.hpp"
#include "../chat_message.hpp"
#include "../chat_channel_ids.hpp"
#include "../chat_message_type_ids.hpp"
#include "../chat_message_slot_ids.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_ChatSendMessage, account_uuid, session, req){
	// TODO quiet

	const auto channel     = ChatChannelId(req.channel);
	const auto type        = ChatMessageTypeId(req.type);
	const auto language_id = LanguageId(req.language_id);

	const auto utc_now = Poseidon::get_utc_time();

	std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
	segments.reserve(req.segments.size());
	for(auto it = req.segments.begin(); it != req.segments.end(); ++it){
		const auto slot = ChatMessageSlotId(it->slot);
		if((slot != ChatMessageSlotIds::ID_TEXT) && (slot != ChatMessageSlotIds::ID_SMILEY) && (slot != ChatMessageSlotIds::ID_VOICE)){
			return Response(Msg::ERR_INVALID_CHAT_MESSAGE_SLOT) <<slot;
		}
		segments.emplace_back(slot, std::move(it->value));
	}

	const auto message = boost::make_shared<ChatMessage>(channel, type, language_id, utc_now, account_uuid, std::move(segments));

	if(channel == ChatChannelIds::ID_WORLD){
		// TODO check flood

		// XXX 附近的人
		boost::container::flat_map<AccountUuid, boost::shared_ptr<PlayerSession>> sessions;
		PlayerSessionMap::get_all(sessions);
		auto it = sessions.begin();
		try {
			while(it != sessions.end()){
				const auto chat_box = ChatBoxMap::get(it->first);
				if(chat_box){
					chat_box->insert(message);
				}
				++it;
			}
		} catch(...){
			while(it != sessions.begin()){
				--it;
				const auto chat_box = ChatBoxMap::get(it->first);
				if(chat_box){
					chat_box->remove(message->get_chat_message_uuid());
				}
			}
			throw;
		}
	} else if(channel == ChatChannelIds::ID_TRADE){
		// TODO check flood
	} else if(channel == ChatChannelIds::ID_ALLIANCE){
		// TODO check flood
		// TODO check alliance
	} else {
		return Response(Msg::ERR_CANNOT_SEND_TO_SYSTEM_CHANNEL) <<channel;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_ChatGetMessages, account_uuid, session, req){
	const auto chat_box = ChatBoxMap::require(account_uuid);

	Msg::SC_ChatGetMessagesRet msg;
	msg.chat_messages.reserve(req.chat_messages.size());

	// const auto language_id = LanguageId(req.language_id);
	for(auto it = req.chat_messages.begin(); it != req.chat_messages.end(); ++it){
		const auto chat_message_uuid = ChatMessageUuid(it->chat_message_uuid);

		msg.chat_messages.emplace_back();
		auto &chat_message = msg.chat_messages.back();
		chat_message.chat_message_uuid = chat_message_uuid.str();
		chat_message.error_code = Msg::ERR_NO_SUCH_CHAT_MESSAGE;

		const auto message = chat_box->get(chat_message_uuid);
		if(!message){
			continue;
		}
		synchronize_chat_message_with_player(message, session);
	}

	return Response();
}

}
