#include "precompiled.hpp"
#include "chat_box.hpp"
#include "chat_message.hpp"
#include "msg/sc_chat.hpp"
#include "singletons/chat_box_map.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/global.hpp"
#include "chat_channel_ids.hpp"

namespace EmperyCenter {

namespace {
	void fill_chat_message(Msg::SC_ChatMessageReceived &msg, const boost::shared_ptr<ChatMessage> &message){
		PROFILE_ME;

		msg.chat_message_uuid = message->get_chat_message_uuid().str();
		msg.channel           = message->get_channel().get();
		msg.type              = message->get_type().get();
		msg.language_id       = message->get_language_id().get();
		msg.created_time      = message->get_created_time();
	}
}

ChatBox::ChatBox(AccountUuid account_uuid)
	: m_account_uuid(account_uuid)
{
}
ChatBox::~ChatBox(){
}

boost::shared_ptr<ChatMessage> ChatBox::get(ChatMessageUuid chat_message_uuid) const {
	PROFILE_ME;

	boost::shared_ptr<ChatMessage> message;
	for(auto cit = m_channels.begin(); cit != m_channels.end(); ++cit){
		const auto it = cit->second.find(chat_message_uuid);
		if(it != cit->second.end()){
			message = it->second;
			break;
		}
	}
	return message;
}
void ChatBox::get_all(std::vector<boost::shared_ptr<ChatMessage>> &ret) const {
	PROFILE_ME;

	std::size_t count_total = 0;
	for(auto cit = m_channels.begin(); cit != m_channels.end(); ++cit){
		count_total += cit->second.size();
	}
	ret.reserve(ret.size() + count_total);
	for(auto cit = m_channels.begin(); cit != m_channels.end(); ++cit){
		for(auto it = cit->second.begin(); it != cit->second.end(); ++it){
			ret.emplace_back(it->second);
		}
	}
}
void ChatBox::get_by_channel(std::vector<boost::shared_ptr<ChatMessage>> &ret, ChatChannelId channel) const {
	PROFILE_ME;

	const auto cit = m_channels.find(channel);
	if(cit != m_channels.end()){
		ret.reserve(ret.size() + cit->second.size());
		for(auto it = cit->second.begin(); it != cit->second.end(); ++it){
			ret.emplace_back(it->second);
		}
	}
}

void ChatBox::insert(const boost::shared_ptr<ChatMessage> &message){
	PROFILE_ME;

	std::uint64_t max_count_in_channel = 0;
	const auto channel = message->get_channel();
	if(channel == ChatChannelIds::ID_ADJACENT){
		max_count_in_channel = Data::Global::as_unsigned(Data::Global::SLOT_MAX_MESSAGES_IN_ADJACENT_CHANNEL);
	} else if(channel == ChatChannelIds::ID_ALLIANCE){
		max_count_in_channel = Data::Global::as_unsigned(Data::Global::SLOT_MAX_MESSAGES_IN_ALLIANCE_CHANNEL);
	} else if(channel == ChatChannelIds::ID_SYSTEM){
		max_count_in_channel = Data::Global::as_unsigned(Data::Global::SLOT_MAX_MESSAGES_IN_SYSTEM_CHANNEL);
	} else if(channel == ChatChannelIds::ID_TRADE){
		max_count_in_channel = Data::Global::as_unsigned(Data::Global::SLOT_MAX_MESSAGES_IN_TRADE_CHANNEL);
	}
	if(max_count_in_channel == 0){
		LOG_EMPERY_CENTER_WARNING("Channel capacity is zero: channel = ", channel);
		return;
	}

	auto &map = m_channels[channel];
	map.reserve(max_count_in_channel + 1);

	const auto chat_message_uuid = message->get_chat_message_uuid();
	const auto result = map.emplace(chat_message_uuid, message);
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Chat message already exists: chat_message_uuid = ", chat_message_uuid);
		DEBUG_THROW(Exception, sslit("Chat message already exists"));
	}
	if(map.size() > max_count_in_channel){
		map.erase(map.begin(), map.begin() + static_cast<std::ptrdiff_t>(max_count_in_channel));
	}

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_ChatMessageReceived msg;
			fill_chat_message(msg, message);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

void ChatBox::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	for(auto cit = m_channels.begin(); cit != m_channels.end(); ++cit){
		for(auto it = cit->second.begin(); it != cit->second.end(); ++it){
			Msg::SC_ChatMessageReceived msg;
			fill_chat_message(msg, it->second);
			session->send(msg);
		}
	}
}

}
