#include "precompiled.hpp"
#include "announcement.hpp"
#include <poseidon/json.hpp>
#include "mysql/announcement.hpp"
#include "player_session.hpp"
#include "msg/sc_announcement.hpp"
#include "singletons/announcement_map.hpp"
#include "checked_arithmetic.hpp"
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
}

Announcement::Announcement(AnnouncementUuid announcement_uuid, LanguageId language_id, boost::uint64_t created_time,
	boost::uint64_t expiry_time, boost::uint64_t period, std::vector<std::pair<ChatMessageSlotId, std::string>> segments)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_Announcement>(announcement_uuid.get(), language_id.get(), created_time,
				expiry_time, period, encode_segments(segments));
			obj->async_save(true);
			return obj;
		}())
	, m_segments(std::move(segments))
{
}
Announcement::Announcement(boost::shared_ptr<MySql::Center_Announcement> obj)
	: m_obj(std::move(obj))
	, m_segments(decode_segments(m_obj->unlocked_get_segments()))
{
}
Announcement::~Announcement(){
}

AnnouncementUuid Announcement::get_announcement_uuid() const {
	return AnnouncementUuid(m_obj->unlocked_get_announcement_uuid());
}
LanguageId Announcement::get_language_id() const {
	return LanguageId(m_obj->get_language_id());
}
boost::uint64_t Announcement::get_created_time() const {
	return m_obj->get_created_time();
}

boost::uint64_t Announcement::get_expiry_time() const {
	return m_obj->get_expiry_time();
}
boost::uint64_t Announcement::get_period() const {
	return m_obj->get_period();
}
const std::vector<std::pair<ChatMessageSlotId, std::string>> &Announcement::get_segments() const {
	return m_segments;
}

void Announcement::modify(boost::uint64_t expiry_time, boost::uint64_t period, std::vector<std::pair<ChatMessageSlotId, std::string>> segments){
	PROFILE_ME;

	auto str = encode_segments(segments);

	m_obj->set_expiry_time(expiry_time);
	m_obj->set_period(period);
	m_obj->set_segments(std::move(str));
	m_segments = std::move(segments);

	AnnouncementMap::update(virtual_shared_from_this<Announcement>(), false);
}
void Announcement::delete_from_game() noexcept {
	PROFILE_ME;

	m_obj->set_expiry_time(0);

	AnnouncementMap::remove(get_announcement_uuid(), get_language_id());
}

void Announcement::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	const auto expiry_duration = saturated_sub(get_expiry_time(), utc_now);
	if(expiry_duration == 0){
		Msg::SC_AnnouncementRemoved msg;
		msg.announcement_uuid = get_announcement_uuid().str();
		session->send(msg);
	} else {
		presend_chat_message_segments(m_segments, session);

		Msg::SC_AnnouncementReceived msg;
		msg.announcement_uuid = get_announcement_uuid().str();
		msg.language_id       = get_language_id().get();
		msg.created_time      = get_created_time();
		msg.expiry_duration   = expiry_duration;
		msg.period            = get_period();
		msg.segments.reserve(m_segments.size());
		for(auto it = m_segments.begin(); it != m_segments.end(); ++it){
			auto &segment = *msg.segments.emplace(msg.segments.end());
			segment.slot  = it->first.get();
			segment.value = it->second;
		}
		session->send(msg);
	}
}

}
