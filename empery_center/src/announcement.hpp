#ifndef EMPERY_CENTER_ANNOUNCEMENT_HPP_
#define EMPERY_CENTER_ANNOUNCEMENT_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/cxx_util.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <vector>
#include <string>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Announcement;
}

class PlayerSession;

class Announcement : public virtual Poseidon::VirtualSharedFromThis {
private:
	const boost::shared_ptr<MySql::Center_Announcement> m_obj;

	std::vector<std::pair<ChatMessageSlotId, std::string>> m_segments;

public:
	Announcement(AnnouncementUuid announcement_uuid, LanguageId language_id, boost::uint64_t created_time,
		boost::uint64_t expiry_time, boost::uint64_t period, std::vector<std::pair<ChatMessageSlotId, std::string>> segments);
	explicit Announcement(boost::shared_ptr<MySql::Center_Announcement> obj);
	~Announcement();

public:
	AnnouncementUuid get_announcement_uuid() const;
	LanguageId get_language_id() const;
	boost::uint64_t get_created_time() const;

	boost::uint64_t get_expiry_time() const;
	boost::uint64_t get_period() const;
	const std::vector<std::pair<ChatMessageSlotId, std::string>> &get_segments() const;

	void modify(boost::uint64_t expiry_time, boost::uint64_t period, std::vector<std::pair<ChatMessageSlotId, std::string>> segments);
	void delete_from_game() noexcept;

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

inline void synchronize_announcement_with_player(const boost::shared_ptr<const Announcement> &announcement,
	const boost::shared_ptr<PlayerSession> &session)
{
	announcement->synchronize_with_player(session);
}
inline void synchronize_announcement_with_player(const boost::shared_ptr<Announcement> &announcement,
	const boost::shared_ptr<PlayerSession> &session)
{
	announcement->synchronize_with_player(session);
}

}

#endif
