#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_chat.hpp"
#include "../msg/sc_chat.hpp"
#include "../msg/err_chat.hpp"
#include "../singletons/chat_message_map.hpp"
#include "../chat_message.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_ChatSendMessage, account_uuid, session, req){
	// TODO quiet

	const unsigned channel = req.channel;
	const unsigned type    = req.type;
	const auto language_id = LanguageId(req.language_id);

	if(channel == ChatMessage::CHAN_SYSTEM){
		return Response(Msg::ERR_CANNOT_SEND_TO_SYSTEM_CHANNEL);
	}

	// TODO check flood

	std::vector<std::pair<unsigned, std::string>> segments;
	segments.reserve(req.segments.size());
	for(auto it = req.segments.begin(); it != req.segments.end(); ++it){
		const unsigned slot = it->slot;
		if((slot != ChatMessage::SLOT_TEXT) && (slot != ChatMessage::SLOT_SMILEY) && (slot != ChatMessage::SLOT_VOICE)){
			return Response(Msg::ERR_INVALID_CHAT_MESSAGE_SLOT) <<slot;
		}
		segments.emplace_back(slot, std::move(it->value));
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto message = boost::make_shared<ChatMessage>(channel, type, language_id,
		account_uuid, utc_now, std::move(segments));
	ChatMessageMap::insert(message);

	return Response();
}

PLAYER_SERVLET(Msg::CS_ChatGetMessages, account_uuid, session, req){
	Msg::SC_ChatGetMessagesRet msg;
	msg.chat_messages.reserve(req.chat_messages.size());

	// const auto language_id = LanguageId(req.language_id);
	for(auto it = req.chat_messages.begin(); it != req.chat_messages.end(); ++it){
		const auto chat_message_uuid = ChatMessageUuid(it->chat_message_uuid);

		msg.chat_messages.emplace_back();
		auto &chat_message = msg.chat_messages.back();
		chat_message.chat_message_uuid = chat_message_uuid.str();
		chat_message.error_code = Msg::ERR_NO_SUCH_CHAT_MESSAGE;

		const auto message = ChatMessageMap::get(chat_message_uuid);
		if(!message){
			continue;
		}
		synchronize_chat_message_with_player(message, session);
	}

	return Response();
}

}
