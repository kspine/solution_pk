#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_announcement.hpp"
#include "../singletons/announcement_map.hpp"
#include "../announcement.hpp"

namespace EmperyCenter {

namespace {
	void fill_announcement_object(Poseidon::JsonObject &object, const boost::shared_ptr<Announcement> &announcement){
		PROFILE_ME;

		object[sslit("announcement_uuid")] = announcement->get_announcement_uuid().str();
		object[sslit("language_id")]       = announcement->get_language_id().get();
		object[sslit("created_time")]      = announcement->get_created_time();
		object[sslit("expiry_time")]       = announcement->get_expiry_time();
		object[sslit("period")]            = announcement->get_period();

		const auto &segments = announcement->get_segments();
		Poseidon::JsonArray temp_array;
		for(auto it = segments.begin(); it != segments.end(); ++it){
			Poseidon::JsonArray elem;
			elem.emplace_back(it->first.get());
			elem.emplace_back(it->second);
			temp_array.emplace_back(std::move(elem));
		}
		object[sslit("segments")] = std::move(temp_array);
	}
}

ADMIN_SERVLET("announcement/get", root, session, params){
	const auto announcement_uuid = AnnouncementUuid(params.at("announcement_uuid"));
	const auto language_id       = boost::lexical_cast<LanguageId>(params.at("language_id"));

	const auto announcement = AnnouncementMap::get(announcement_uuid, language_id);
	if(!announcement){
		return Response(Msg::ERR_NO_SUCH_ANNOUNCEMENT) <<announcement_uuid <<',' <<language_id;
	}

	Poseidon::JsonObject object;
	fill_announcement_object(object, announcement);
	root[sslit("announcement")] = std::move(object);

	return Response();
}

ADMIN_SERVLET("announcement/get_all", root, session, /* params */){
	std::vector<boost::shared_ptr<Announcement>> announcements;
	AnnouncementMap::get_all(announcements);

	Poseidon::JsonArray temp_array;
	for(auto it = announcements.begin(); it != announcements.end(); ++it){
		Poseidon::JsonObject object;
		fill_announcement_object(object, *it);
		temp_array.emplace_back(std::move(object));
	}
	root[sslit("announcements")] = std::move(temp_array);

	return Response();
}

ADMIN_SERVLET("announcement/insert", root, session, params){
	const auto language_id = boost::lexical_cast<LanguageId>     (params.at("language_id"));
	const auto expiry_time = boost::lexical_cast<std::uint64_t>(params.at("expiry_time"));
	const auto period      = boost::lexical_cast<std::uint64_t>(params.at("period"));

	std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
	std::istringstream iss(params.at("segments"));
	auto temp_array = Poseidon::JsonParser::parse_array(iss);
	segments.reserve(temp_array.size());
	for(auto it = temp_array.begin(); it != temp_array.end(); ++it){
		auto &elem = it->get<Poseidon::JsonArray>();
		auto slot_id = ChatMessageSlotId(elem.at(0).get<double>());
		auto value   = std::move(elem.at(1).get<std::string>());
		segments.emplace_back(slot_id, std::move(value));
	}

	const auto announcement_uuid = AnnouncementUuid(Poseidon::Uuid::random());
	const auto utc_now = Poseidon::get_utc_time();
	const auto announcement = boost::make_shared<Announcement>(
		announcement_uuid, language_id, utc_now, expiry_time, period, std::move(segments));
	AnnouncementMap::insert(announcement);

	return Response();
}

ADMIN_SERVLET("announcement/update", root, session, params){
	const auto announcement_uuid = AnnouncementUuid(params.at("announcement_uuid"));
	const auto language_id       = boost::lexical_cast<LanguageId>(params.at("language_id"));

	const auto announcement = AnnouncementMap::get(announcement_uuid, language_id);
	if(!announcement){
		return Response(Msg::ERR_NO_SUCH_ANNOUNCEMENT) <<announcement_uuid <<',' <<language_id;
	}

	const auto expiry_time = boost::lexical_cast<std::uint64_t>(params.at("expiry_time"));
	const auto period      = boost::lexical_cast<std::uint64_t>(params.at("period"));

	std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
	std::istringstream iss(params.at("segments"));
	auto temp_array = Poseidon::JsonParser::parse_array(iss);
	segments.reserve(temp_array.size());
	for(auto it = temp_array.begin(); it != temp_array.end(); ++it){
		auto &elem = it->get<Poseidon::JsonArray>();
		auto slot_id = ChatMessageSlotId(elem.at(0).get<double>());
		auto value   = std::move(elem.at(1).get<std::string>());
		segments.emplace_back(slot_id, std::move(value));
	}

	announcement->modify(expiry_time, period, std::move(segments));

	return Response();
}

ADMIN_SERVLET("announcement/remove", root, session, params){
	const auto announcement_uuid = AnnouncementUuid(params.at("announcement_uuid"));
	const auto language_id       = boost::lexical_cast<LanguageId>(params.at("language_id"));

	const auto announcement = AnnouncementMap::get(announcement_uuid, language_id);
	if(!announcement){
		return Response(Msg::ERR_NO_SUCH_ANNOUNCEMENT) <<announcement_uuid <<',' <<language_id;
	}

	announcement->delete_from_game();

	return Response();
}

}
