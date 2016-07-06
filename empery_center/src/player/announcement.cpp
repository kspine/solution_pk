#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_announcement.hpp"
#include "../msg/err_announcement.hpp"
#include "../singletons/announcement_map.hpp"
#include "../announcement.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_AnnouncementGetAnnouncements, account, session, req){
	const auto language_id = LanguageId(req.language_id);

	std::vector<boost::shared_ptr<Announcement>> announcements;
	AnnouncementMap::get_by_language_id(announcements, language_id);
	AnnouncementMap::get_by_language_id(announcements, LanguageId());
	for(auto it = announcements.begin(); it != announcements.end(); ++it){
		const auto &announcement = *it;
		synchronize_announcement_with_player(announcement, session);
	}

	return Response();
}

}
