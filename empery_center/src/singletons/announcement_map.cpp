#include "../precompiled.hpp"
#include "announcement_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "player_session_map.hpp"
#include "../mysql/announcement.hpp"
#include "../announcement.hpp"
#include "../player_session.hpp"

namespace EmperyCenter {

namespace {
	struct AnnouncementElement {
		boost::shared_ptr<Announcement> announcement;

		AnnouncementUuid announcement_uuid;
		LanguageId language_id;

		explicit AnnouncementElement(boost::shared_ptr<Announcement> announcement_)
			: announcement(std::move(announcement_))
			, announcement_uuid(announcement->get_announcement_uuid()), language_id(announcement->get_language_id())
		{
		}
	};

	MULTI_INDEX_MAP(AnnouncementContainer, AnnouncementElement,
		UNIQUE_MEMBER_INDEX(announcement_uuid)
		MULTI_MEMBER_INDEX(language_id)
	)

	boost::weak_ptr<AnnouncementContainer> g_announcement_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();
		const auto utc_now = Poseidon::get_utc_time();

		const auto announcement_map = boost::make_shared<AnnouncementContainer>();
		LOG_EMPERY_CENTER_INFO("Loading announcements...");
		conn->execute_sql("SELECT * FROM `Center_Announcement`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_Announcement>();
			obj->fetch(conn);
			if(obj->get_expiry_time() < utc_now){
				continue;
			}
			obj->enable_auto_saving();
			auto announcement = boost::make_shared<Announcement>(std::move(obj));
			announcement_map->insert(AnnouncementElement(std::move(announcement)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", announcement_map->size(), " announcement(s).");
		g_announcement_map = announcement_map;
		handles.push(announcement_map);
	}

	void synchronize_announcement_all(const boost::shared_ptr<Announcement> &announcement) noexcept
	try {
		PROFILE_ME;

		std::vector<std::pair<boost::shared_ptr<Account>, boost::shared_ptr<PlayerSession>>> online_players;
		PlayerSessionMap::get_all(online_players);
		for(auto it = online_players.begin(); it != online_players.end(); ++it){
			const auto &session = it->second;
			try {
				synchronize_announcement_with_player(announcement, session);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		PlayerSessionMap::clear(e.what());
	}
}

boost::shared_ptr<Announcement> AnnouncementMap::get(AnnouncementUuid announcement_uuid){
	PROFILE_ME;

	const auto announcement_map = g_announcement_map.lock();
	if(!announcement_map){
		LOG_EMPERY_CENTER_WARNING("Announcement is not loaded.");
		return { };
	}

	const auto it = announcement_map->find<0>(announcement_uuid);
	if(it == announcement_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Announcement not found: announcement_uuid = ", announcement_uuid);
		return { };
	}
	return it->announcement;
}
boost::shared_ptr<Announcement> AnnouncementMap::require(AnnouncementUuid announcement_uuid){
	PROFILE_ME;

	auto ret = get(announcement_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Announcement not found: announcement_uuid = ", announcement_uuid);
		DEBUG_THROW(Exception, sslit("Announcement not found"));
	}
	return ret;
}

void AnnouncementMap::get_all(std::vector<boost::shared_ptr<Announcement>> &ret){
	PROFILE_ME;

	const auto announcement_map = g_announcement_map.lock();
	if(!announcement_map){
		LOG_EMPERY_CENTER_WARNING("Announcement is not loaded.");
		return;
	}

	ret.reserve(ret.size() + announcement_map->size());
	for(auto it = announcement_map->begin<0>(); it != announcement_map->end<0>(); ++it){
		ret.emplace_back(it->announcement);
	}
}
void AnnouncementMap::get_by_language_id(std::vector<boost::shared_ptr<Announcement>> &ret, LanguageId language_id){
	PROFILE_ME;

	const auto announcement_map = g_announcement_map.lock();
	if(!announcement_map){
		LOG_EMPERY_CENTER_WARNING("Announcement is not loaded.");
		return;
	}

	const auto range = announcement_map->equal_range<1>(language_id);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->announcement);
	}
}

void AnnouncementMap::insert(const boost::shared_ptr<Announcement> &announcement){
	PROFILE_ME;

	const auto announcement_map = g_announcement_map.lock();
	if(!announcement_map){
		LOG_EMPERY_CENTER_WARNING("Announcement is not loaded.");
		DEBUG_THROW(Exception, sslit("Announcement map not loaded"));
	}

	const auto announcement_uuid = announcement->get_announcement_uuid();

	LOG_EMPERY_CENTER_DEBUG("Inserting announcement: announcement_uuid = ", announcement_uuid);
	if(!announcement_map->insert(AnnouncementElement(announcement)).second){
		LOG_EMPERY_CENTER_WARNING("Announcement already exists: announcement_uuid = ", announcement_uuid);
		DEBUG_THROW(Exception, sslit("Announcement already exists"));
	}

	synchronize_announcement_all(announcement);
}
void AnnouncementMap::update(const boost::shared_ptr<Announcement> &announcement, bool throws_if_not_exists){
	PROFILE_ME;

	const auto announcement_map = g_announcement_map.lock();
	if(!announcement_map){
		LOG_EMPERY_CENTER_WARNING("Announcement map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Announcement map not loaded"));
		}
		return;
	}

	const auto announcement_uuid = announcement->get_announcement_uuid();

	const auto it = announcement_map->find<0>(announcement_uuid);
	if(it == announcement_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Announcement not found: announcement_uuid = ", announcement_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Announcement not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating announcement: announcement_uuid = ", announcement_uuid);

	synchronize_announcement_all(announcement);
}
void AnnouncementMap::remove(AnnouncementUuid announcement_uuid) noexcept {
	PROFILE_ME;

	const auto announcement_map = g_announcement_map.lock();
	if(!announcement_map){
		LOG_EMPERY_CENTER_WARNING("Announcement map not loaded.");
		return;
	}

	const auto it = announcement_map->find<0>(announcement_uuid);
	if(it == announcement_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Announcement not found: announcement_uuid = ", announcement_uuid);
		return;
	}
	const auto announcement = it->announcement;

	LOG_EMPERY_CENTER_DEBUG("Removing announcement: announcement_uuid = ", announcement_uuid);
	announcement_map->erase<0>(it);

	synchronize_announcement_all(announcement);
}

}
