#ifndef EMPERY_CENTER_ANNOUNCEMENT_HPP_
#define EMPERY_CENTER_ANNOUNCEMENT_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Announcement;
}

class PlayerSession;

class Announcement : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const boost::shared_ptr<MySql::Center_Announcement> m_obj;

	std::vector<std::pair<ChatMessageSlotId, std::string>> m_segments;

public:
	Announcement(AnnouncementUuid announcement_uuid, LanguageId language_id, std::uint64_t created_time,
		std::uint64_t expiry_time, std::uint64_t period,
		ChatMessageTypeId type, std::vector<std::pair<ChatMessageSlotId, std::string>> segments);
	explicit Announcement(boost::shared_ptr<MySql::Center_Announcement> obj);
	~Announcement();

public:
	AnnouncementUuid get_announcement_uuid() const;
	LanguageId get_language_id() const;
	std::uint64_t get_created_time() const;

	std::uint64_t get_expiry_time() const;
	std::uint64_t get_period() const;
	ChatMessageTypeId get_type() const;
	const std::vector<std::pair<ChatMessageSlotId, std::string>> &get_segments() const;

	void modify(std::uint64_t expiry_time, std::uint64_t period,
		ChatMessageTypeId type, std::vector<std::pair<ChatMessageSlotId, std::string>> segments);
	void delete_from_game() noexcept;

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

}

#endif
