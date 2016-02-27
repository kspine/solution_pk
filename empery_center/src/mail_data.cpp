#include "precompiled.hpp"
#include "mail_data.hpp"
#include <poseidon/json.hpp>
#include "mysql/mail.hpp"
#include "player_session.hpp"
#include "msg/sc_mail.hpp"
#include "singletons/player_session_map.hpp"
#include "singletons/account_map.hpp"
#include "chat_message.hpp"

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

	std::string encode_attachments(const boost::container::flat_map<ItemId, std::uint64_t> &attachments){
		PROFILE_ME;

		if(attachments.empty()){
			return { };
		}
		Poseidon::JsonObject root;
		for(auto it = attachments.begin(); it != attachments.end(); ++it){
			const auto item_id = it->first;
			const auto item_count = it->second;
			char str[64];
			unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)item_id.get());
			root[SharedNts(str, len)] = item_count;
		}
		std::ostringstream oss;
		root.dump(oss);
		return oss.str();
	}
	boost::container::flat_map<ItemId, std::uint64_t> decode_attachments(const std::string &str){
		PROFILE_ME;

		boost::container::flat_map<ItemId, std::uint64_t> attachments;
		if(str.empty()){
			return attachments;
		}
		std::istringstream iss(str);
		auto root = Poseidon::JsonParser::parse_object(iss);
		attachments.reserve(root.size());
		for(auto it = root.begin(); it != root.end(); ++it){
			auto item_id = boost::lexical_cast<ItemId>(it->first);
			auto item_count = static_cast<std::uint64_t>(it->second.get<double>());
			attachments[item_id] = item_count;
		}
		return attachments;
	}
}

MailData::MailData(MailUuid mail_uuid, LanguageId language_id, std::uint64_t created_time,
	MailTypeId type, AccountUuid from_account_uuid, std::string subject,
	std::vector<std::pair<ChatMessageSlotId, std::string>> segments, boost::container::flat_map<ItemId, std::uint64_t> attachments)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_MailData>(
				mail_uuid.get(), language_id.get(), created_time, type.get(), from_account_uuid.get(), std::move(subject),
				encode_segments(segments), encode_attachments(attachments));
			obj->async_save(true);
			return obj;
		}())
	, m_segments(std::move(segments)), m_attachments(std::move(attachments))
{
}
MailData::MailData(boost::shared_ptr<MySql::Center_MailData> obj)
	: m_obj(std::move(obj))
	, m_segments(decode_segments(m_obj->unlocked_get_segments())), m_attachments(decode_attachments(m_obj->unlocked_get_attachments()))
{
}
MailData::~MailData(){
}

MailUuid MailData::get_mail_uuid() const {
	return MailUuid(m_obj->unlocked_get_mail_uuid());
}
LanguageId MailData::get_language_id() const {
	return LanguageId(m_obj->get_language_id());
}
std::uint64_t MailData::get_created_time() const {
	return m_obj->get_created_time();
}

MailTypeId MailData::get_type() const {
	return MailTypeId(m_obj->get_type());
}
void MailData::set_type(MailTypeId type){
	m_obj->set_type(type.get());
}

AccountUuid MailData::get_from_account_uuid() const {
	return AccountUuid(m_obj->unlocked_get_from_account_uuid());
}
void MailData::set_from_account_uuid(AccountUuid account_uuid){
	m_obj->set_from_account_uuid(account_uuid.get());
}

const std::string &MailData::get_subject() const {
	return m_obj->unlocked_get_subject();
}
void MailData::set_subject(std::string subject){
	m_obj->set_subject(std::move(subject));
}

const std::vector<std::pair<ChatMessageSlotId, std::string>> &MailData::get_segments() const {
	return m_segments;
}
void MailData::set_segments(std::vector<std::pair<ChatMessageSlotId, std::string>> segments){
	PROFILE_ME;

	auto str = encode_segments(segments);
	m_segments = std::move(segments);
	m_obj->set_segments(std::move(str));
}

const boost::container::flat_map<ItemId, std::uint64_t> &MailData::get_attachments() const {
	return m_attachments;
}
void MailData::set_attachments(boost::container::flat_map<ItemId, std::uint64_t> attachments){
	PROFILE_ME;

	auto str = encode_attachments(attachments);
	m_attachments = std::move(attachments);
	m_obj->set_attachments(std::move(str));
}

void MailData::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto from_account_uuid = get_from_account_uuid();
	if(from_account_uuid){
		AccountMap::cached_synchronize_account_with_player(from_account_uuid, session);
	}

	presend_chat_message_segments(m_segments, session);

	Msg::SC_MailData msg;
	msg.mail_uuid         = get_mail_uuid().str();
	msg.language_id       = get_language_id().get();
	msg.created_time      = get_created_time();
	msg.type              = get_type().get();
	msg.from_account_uuid = from_account_uuid.str();
	msg.subject           = get_subject();
	msg.segments.reserve(m_segments.size());
	for(auto it = m_segments.begin(); it != m_segments.end(); ++it){
		auto &segment = *msg.segments.emplace(msg.segments.end());
		segment.slot  = it->first.get();
		segment.value = it->second;
	}
	msg.attachments.reserve(m_attachments.size());
	for(auto it = m_attachments.begin(); it != m_attachments.end(); ++it){
		auto &attachment = *msg.attachments.emplace(msg.attachments.end());
		attachment.item_id    = it->first.get();
		attachment.item_count = it->second;
	}
	session->send(msg);
}

}
