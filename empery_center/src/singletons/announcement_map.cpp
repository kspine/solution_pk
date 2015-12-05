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

		explicit AnnouncementElement(boost::shared_ptr<Announcement> announcement_)
			: announcement(std::move(announcement_))
		{
		}
	};

	MULTI_INDEX_MAP(AnnouncementMapContainer, AnnouncementElement,
		UNIQUE_MEMBER_INDEX(announcement_uuid)
	)

	boost::weak_ptr<AnnouncementMapContainer> g_announcement_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto announcement_map = boost::make_shared<AnnouncementMapContainer>();
		LOG_EMPERY_CENTER_INFO("Loading announcements...");
		conn->execute_sql("SELECT * FROM `Center_Announcement`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_Announcement>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			auto announcement = boost::make_shared<Announcement>(std::move(obj));
			announcement_map->insert(AnnouncementElement(std::move(announcement)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", announcement_map->size(), " announcement(s).");
		g_announcement_map = announcement_map;
		handles.push(announcement_map);
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
		LOG_EMPERY_CENTER_DEBUG("Announcement not found: announcement_uuid = ", announcement_uuid);
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

void AnnouncementMap::insert(const boost::shared_ptr<Announcement> &announcement){
}
void AnnouncementMap::update(const boost::shared_ptr<Announcement> &announcement, bool throws_if_not_exists){
}
void AnnouncementMap::remove(AnnouncementUuid announcement_uuid) noexcept {
}

}
