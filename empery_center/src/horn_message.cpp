#include "precompiled.hpp"
#include "horn_message.hpp"
#include "chat_message.hpp"
#include <poseidon/json.hpp>
#include "player_session.hpp"
#include "mysql/horn_message.hpp"
#include "msg/sc_chat.hpp"
#include "singletons/chat_box_map.hpp"
#include "singletons/player_session_map.hpp"
#include "singletons/account_map.hpp"
#include "chat_message_slot_ids.hpp"

namespace EmperyCenter {

namespace {
	std::string encode_segments(const std::vector<std::pair<ChatMessageSlotId, std::string>> &segments){
		PROFILE_ME;

		if(segments.empty()){
			return { };
		}
		Poseidon::JsonArray root;
		for(auto it = segments.begin(); it != segments.end(); ++it){
			const auto slot = it->first;
			const auto &value = it->second;
			Poseidon::JsonArray elem;
			elem.emplace_back(slot.get());
			elem.emplace_back(value);
			root.emplace_back(std::move(elem));
		}
		std::ostringstream oss;
		root.dump(oss);
		return oss.str();
	}
	std::vector<std::pair<ChatMessageSlotId, std::string>> decode_segments(const std::string &str){
		PROFILE_ME;

		std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
		if(str.empty()){
			return segments;
		}
		std::istringstream iss(str);
		auto root = Poseidon::JsonParser::parse_array(iss);
		segments.reserve(root.size());
		for(auto it = root.begin(); it != root.end(); ++it){
			auto elem = it->get<Poseidon::JsonArray>();
			auto slot = ChatMessageSlotId(elem.at(0).get<double>());
			auto value = std::move(elem.at(1)).get<std::string>();
			segments.emplace_back(slot, std::move(value));
		}
		return segments;
	}
}

HornMessage::HornMessage(HornMessageUuid horn_message_uuid,
	LanguageId language_id, std::uint64_t created_time, std::uint64_t expiry_time,
	AccountUuid from_account_uuid, std::vector<std::pair<ChatMessageSlotId, std::string>> segments)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_HornMessage>(
				horn_message_uuid.get(), language_id.get(), created_time, expiry_time,
				from_account_uuid.get(), encode_segments(segments));
			obj->async_save(true, true);
			return obj;
		}())
	, m_segments(std::move(segments))
{
}
HornMessage::HornMessage(boost::shared_ptr<MySql::Center_HornMessage> obj)
	: m_obj(std::move(obj))
	, m_segments(decode_segments(m_obj->unlocked_get_segments()))
{
}
HornMessage::~HornMessage(){
}

HornMessageUuid HornMessage::get_horn_message_uuid() const {
	return HornMessageUuid(m_obj->unlocked_get_horn_message_uuid());
}
LanguageId HornMessage::get_language_id() const {
	return LanguageId(m_obj->get_language_id());
}
std::uint64_t HornMessage::get_created_time() const {
	return m_obj->get_created_time();
}
std::uint64_t HornMessage::get_expiry_time() const {
	return m_obj->get_expiry_time();
}
AccountUuid HornMessage::get_from_account_uuid() const {
	return AccountUuid(m_obj->unlocked_get_from_account_uuid());
}

void HornMessage::delete_from_game() noexcept {
	PROFILE_ME;

	m_obj->set_expiry_time(0);

	ChatBoxMap::update_horn_message(virtual_shared_from_this<HornMessage>(), false);
}

bool HornMessage::is_virtually_removed() const {
	return get_expiry_time() == 0;
}
void HornMessage::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto from_account_uuid = get_from_account_uuid();
	if(from_account_uuid){
		AccountMap::cached_synchronize_account_with_player_all(from_account_uuid, session);
	}

	Msg::SC_ChatHornMessage msg;
	msg.horn_message_uuid = get_horn_message_uuid().str();
	msg.language_id                 = get_language_id().get();
	msg.created_time                = get_created_time();
	msg.expiry_time                 = get_expiry_time();
	msg.from_account_uuid           = from_account_uuid.str();
	msg.segments.reserve(m_segments.size());
	for(auto it = m_segments.begin(); it != m_segments.end(); ++it){
		auto &elem = *msg.segments.emplace(msg.segments.end());
		elem.slot  = it->first.get();
		elem.value = it->second;
	}
	session->send(msg);
}

}
