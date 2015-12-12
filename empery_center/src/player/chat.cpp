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
#include "../account.hpp"
#include "../account_attribute_ids.hpp"
#include "../data/global.hpp"
#include "../singletons/world_map.hpp"

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

	const auto account = AccountMap::require(account_uuid);

	boost::uint64_t last_chat_time;
	boost::uint64_t min_seconds;
	if(channel == ChatChannelIds::ID_ADJACENT){
		last_chat_time = account->cast_attribute<boost::uint64_t>(AccountAttributeIds::ID_LAST_CHAT_TIME_ADJACENT);
		min_seconds = Data::Global::as_unsigned(Data::Global::SLOT_MIN_MESSAGE_INTERVAL_IN_ADJACENT_CHANNEL);
	} else if(channel == ChatChannelIds::ID_TRADE){
		last_chat_time = account->cast_attribute<boost::uint64_t>(AccountAttributeIds::ID_LAST_CHAT_TIME_TRADE);
		min_seconds = Data::Global::as_unsigned(Data::Global::SLOT_MIN_MESSAGE_INTERVAL_IN_TRADE_CHANNEL);
	} else if(channel == ChatChannelIds::ID_ALLIANCE){
		last_chat_time = account->cast_attribute<boost::uint64_t>(AccountAttributeIds::ID_LAST_CHAT_TIME_ALLIANCE);
		min_seconds = Data::Global::as_unsigned(Data::Global::SLOT_MIN_MESSAGE_INTERVAL_IN_ALLIANCE_CHANNEL);
	} else {
		return Response(Msg::ERR_CANNOT_SEND_TO_SYSTEM_CHANNEL) <<channel;
	}
	const auto milliseconds_remaining = saturated_sub(
		saturated_mul<boost::uint64_t>(min_seconds, 1000), saturated_sub(utc_now, last_chat_time));
	if(milliseconds_remaining != 0){
		return Response(Msg::ERR_CHAT_FLOOD) <<milliseconds_remaining;
	}

	if(channel == ChatChannelIds::ID_ALLIANCE){
		// TODO check alliance
	}

	const auto chat_message_uuid = ChatMessageUuid(Poseidon::Uuid::random());
	const auto message = boost::make_shared<ChatMessage>(
		chat_message_uuid, channel, type, language_id, utc_now, account_uuid, std::move(segments));

	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers[AccountAttributeIds::ID_LAST_CHAT_TIME_ALLIANCE] = boost::lexical_cast<std::string>(utc_now);
	account->set_attributes(std::move(modifiers));

	if(channel == ChatChannelIds::ID_ADJACENT){
		std::vector<boost::shared_ptr<PlayerSession>> sessions;
		WorldMap::get_players_viewing_rectangle(sessions, Rectangle(req.x, req.y, 1, 1));
		for(auto it = sessions.begin(); it != sessions.end(); ++it){
			const auto &session = *it;
			try {
				const auto other_account_uuid = PlayerSessionMap::require_account_uuid(session);
				const auto other_chat_box = ChatBoxMap::require(other_account_uuid);
				other_chat_box->insert(message);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	} else if(channel == ChatChannelIds::ID_TRADE){
	} else if(channel == ChatChannelIds::ID_ALLIANCE){
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
