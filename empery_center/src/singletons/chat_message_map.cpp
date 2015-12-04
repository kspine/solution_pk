 #include "../precompiled.hpp"
#include "chat_message_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include "../chat_message.hpp"
#include "player_session_map.hpp"
#include "../player_session.hpp"
#include "../msg/sc_chat.hpp"

namespace EmperyCenter {

namespace {
	struct ChatMessageElement {
		boost::shared_ptr<ChatMessage> message;

		ChatMessageUuid message_uuid;
		std::pair<ChatChannelId, boost::uint64_t> channel_sent_time;

		explicit ChatMessageElement(boost::shared_ptr<ChatMessage> message_)
			: message(std::move(message_))
			, message_uuid(message->get_chat_message_uuid()), channel_sent_time(message->get_channel(), message->get_sent_time())
		{
		}
	};

	MULTI_INDEX_MAP(ChatMessageMapDelegator, ChatMessageElement,
		UNIQUE_MEMBER_INDEX(message_uuid)
		MULTI_MEMBER_INDEX(channel_sent_time)
	)

	boost::weak_ptr<ChatMessageMapDelegator> g_chat_message_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto chat_message_map = boost::make_shared<ChatMessageMapDelegator>();
		g_chat_message_map = chat_message_map;
		handles.push(chat_message_map);
	}
}

boost::shared_ptr<ChatMessage> ChatMessageMap::get(ChatMessageUuid chat_message_uuid){
	PROFILE_ME;

	const auto chat_message_map = g_chat_message_map.lock();
	if(!chat_message_map){
		LOG_EMPERY_CENTER_WARNING("Chat message map is gone?");
		return { };
	}

	const auto it = chat_message_map->find<0>(chat_message_uuid);
	if(it == chat_message_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Chat message not found: chat_message_uuid = ", chat_message_uuid);
		return { };
	}
	return it->message;
}
boost::shared_ptr<ChatMessage> ChatMessageMap::require(ChatMessageUuid chat_message_uuid){
	PROFILE_ME;

	auto ret = get(chat_message_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Chat message not found: chat_message_uuid = ", chat_message_uuid);
		DEBUG_THROW(Exception, sslit("Chat message not found"));
	}
	return ret;
}
void ChatMessageMap::get_all_by_channel(std::vector<boost::shared_ptr<ChatMessage>> &ret, ChatChannelId channel){
	PROFILE_ME;

	const auto chat_message_map = g_chat_message_map.lock();
	if(!chat_message_map){
		LOG_EMPERY_CENTER_WARNING("Chat message map is gone?");
		return;
	}

	const auto lower = chat_message_map->lower_bound<1>(std::make_pair(channel, 0));
	const auto upper = chat_message_map->upper_bound<1>(std::make_pair(channel, UINT64_MAX));
	const auto count = static_cast<std::size_t>(std::distance(lower, upper));
	ret.reserve(ret.size() + count);
	for(auto it = lower; it != upper; ++it){
		ret.emplace_back(it->message);
	}
}

void ChatMessageMap::insert(const boost::shared_ptr<ChatMessage> &message){
	PROFILE_ME;

	const auto chat_message_map = g_chat_message_map.lock();
	if(!chat_message_map){
		LOG_EMPERY_CENTER_WARNING("Chat message map is gone?");
		DEBUG_THROW(Exception, sslit("Chat message map is gone"));
	}

	const auto chat_message_uuid = message->get_chat_message_uuid();
	const auto channel = message->get_channel();

	const auto it = chat_message_map->find<0>(chat_message_uuid);
	if(it != chat_message_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Chat message already exists: chat_message_uuid = ", chat_message_uuid);
		DEBUG_THROW(Exception, sslit("Chat message already exists"));
	}
	chat_message_map->insert<0>(ChatMessageElement(message));

	// 删除频道内过剩的消息。
	const auto max_messages_per_channel = get_config<std::size_t>("max_number_of_chat_messages_per_channel", 100);

	const auto lower = chat_message_map->lower_bound<1>(std::make_pair(channel, 0));
	const auto upper = chat_message_map->upper_bound<1>(std::make_pair(channel, UINT64_MAX));
	auto channel_it = upper;
	std::size_t count = 0;
	while((channel_it != lower) && (count < max_messages_per_channel)){
		--channel_it;
		++count;
	}
	chat_message_map->erase<1>(lower, channel_it);

	try {
		boost::container::flat_map<AccountUuid, boost::shared_ptr<PlayerSession>> sessions;
		PlayerSessionMap::get_all(sessions);

		for(auto it = sessions.begin(); it != sessions.end(); ++it){
			const auto &session = it->second;
			try {
				Msg::SC_ChatMessageReceived msg;
				msg.chat_message_uuid     = chat_message_uuid.str();
				msg.channel               = message->get_channel().get();
				msg.type                  = message->get_type().get();
				msg.language_id           = message->get_language_id().get();
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}
}
void ChatMessageMap::remove(ChatMessageUuid chat_message_uuid) noexcept {
	PROFILE_ME;

	const auto chat_message_map = g_chat_message_map.lock();
	if(!chat_message_map){
		LOG_EMPERY_CENTER_WARNING("Chat message map is gone?");
		return;
	}

	const auto it = chat_message_map->find<0>(chat_message_uuid);
	if(it == chat_message_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Chat message not found: chat_message_uuid = ", chat_message_uuid);
		return;
	}
	chat_message_map->erase<0>(it);
}

}
