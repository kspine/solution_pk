#ifndef EMPERY_CENTER_SINGLETONS_ANNOUNCEMENT_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_ANNOUNCEMENT_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <vector>
#include "../id_types.hpp"

namespace EmperyCenter {

class Announcement;

struct AnnouncementMap {
	static boost::shared_ptr<Announcement> get(AnnouncementUuid announcement_uuid, LanguageId language_id);
	static boost::shared_ptr<Announcement> require(AnnouncementUuid announcement_uuid, LanguageId language_id);

	static void get_all(std::vector<boost::shared_ptr<Announcement>> &ret);
	static void get_all_by_announcement_uuid(std::vector<boost::shared_ptr<Announcement>> &ret, AnnouncementUuid announcement_uuid);
	static void get_all_by_language_id(std::vector<boost::shared_ptr<Announcement>> &ret, LanguageId language_id);

	static void insert(const boost::shared_ptr<Announcement> &announcement);
	static void update(const boost::shared_ptr<Announcement> &announcement, bool throws_if_not_exists = true);
	static void remove(AnnouncementUuid announcement_uuid, LanguageId language_id) noexcept;

private:
	AnnouncementMap() = delete;
};

}

#endif
