#include "precompiled.hpp"
#include "chat_box.hpp"
#include "chat_message.hpp"
#include <poseidon/multi_index_map.hpp>
#include "msg/sc_chat.hpp"
#include "singletons/chat_box_map.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/global.hpp"
#include "chat_channel_ids.hpp"

namespace EmperyCenter {

namespace {
	struct MessageElement {
		boost::shared_ptr<ChatMessage> message;

		ChatMessageUuid chat_message_uuid;
		std::pair<ChatChannelId, std::uint64_t> channel_time;

		explicit MessageElement(boost::shared_ptr<ChatMessage> message_)
			: message(std::move(message_))
			, chat_message_uuid(message->get_chat_message_uuid()), channel_time(message->get_channel(), message->get_created_time())
		{
		}
	};

	MULTI_INDEX_MAP(MessageMapContainer, MessageElement,
		UNIQUE_MEMBER_INDEX(chat_message_uuid)
		MULTI_MEMBER_INDEX(channel_time)
	)

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

	const auto messages = boost::static_pointer_cast<MessageMapContainer>(m_messages);
	if(!messages){
		return { };
	}

	const auto it = messages->find<0>(chat_message_uuid);
	if(it == messages->end<0>()){
		return { };
	}
	return it->message;
}
void ChatBox::get_all(std::vector<boost::shared_ptr<ChatMessage>> &ret) const {
	PROFILE_ME;

	const auto messages = boost::static_pointer_cast<MessageMapContainer>(m_messages);
	if(!messages){
		return;
	}

	ret.reserve(ret.size() + messages->size());
	for(auto it = messages->begin<1>(); it != messages->end<1>(); ++it){
		ret.emplace_back(it->message);
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

	auto messages = boost::static_pointer_cast<MessageMapContainer>(m_messages);
	if(!messages){
		messages = boost::make_shared<MessageMapContainer>();
		m_messages = messages;
	}

	const auto chat_message_uuid = message->get_chat_message_uuid();
	const auto result = messages->insert(MessageElement(message));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Chat message exists: account_uuid = ", get_account_uuid(), ", chat_message_uuid = ", chat_message_uuid);
		DEBUG_THROW(Exception, sslit("Chat message exists"));
	}

	const auto channel_lower = messages->lower_bound<1>(std::make_pair(channel, 0));
	auto channel_upper = messages->upper_bound<1>(std::make_pair(channel, UINT64_MAX));
	std::size_t count = 0;
	for(;;){
		if(channel_upper == channel_lower){
			break;
		}
		if(count >= max_count_in_channel){
			messages->erase<1>(channel_lower, channel_upper);
			break;
		}
		--channel_upper;
		++count;
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
bool ChatBox::remove(ChatMessageUuid chat_message_uuid) noexcept {
	PROFILE_ME;

	const auto messages = boost::static_pointer_cast<MessageMapContainer>(m_messages);
	if(!messages){
		return false;
	}

	const auto it = messages->find<0>(chat_message_uuid);
	if(it == messages->end<0>()){
		return false;
	}
	// const auto message = it->message;
	messages->erase(it);

	return true;
}

void ChatBox::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto messages = boost::static_pointer_cast<MessageMapContainer>(m_messages);
	if(!messages){
		return;
	}

	for(auto it = messages->begin<1>(); it != messages->end<1>(); ++it){
		Msg::SC_ChatMessageReceived msg;
		fill_chat_message(msg, it->message);
		session->send(msg);
	}
}

}
