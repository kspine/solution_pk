#ifndef EMPERY_CENTER_SINGLETONS_ANNOUNCEMENT_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_ANNOUNCEMENT_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class Announcement;

struct AnnouncementMap {
	static boost::shared_ptr<Announcement> get(AnnouncementUuid announcement_uuid);
	static boost::shared_ptr<Announcement> require(AnnouncementUuid announcement_uuid);

	static void insert(const boost::shared_ptr<Announcement> &announcement);
	static void update(const boost::shared_ptr<Announcement> &announcement, bool throws_if_not_exists = true);
	static void remove(AnnouncementUuid announcement_uuid) noexcept;

private:
	AnnouncementMap() = delete;
};

}

#endif
