#ifndef EMPERY_CENTER_HORN_MESSAGE_HPP_
#define EMPERY_CENTER_HORN_MESSAGE_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_HornMessage;
}

class PlayerSession;

class HornMessage : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const boost::shared_ptr<MySql::Center_HornMessage> m_obj;

	std::vector<std::pair<ChatMessageSlotId, std::string>> m_segments;

public:
	HornMessage(HornMessageUuid horn_message_uuid, ItemId item_id,
		LanguageId language_id, std::uint64_t created_time, std::uint64_t expiry_time,
		AccountUuid from_account_uuid, std::vector<std::pair<ChatMessageSlotId, std::string>> segments);
	explicit HornMessage(boost::shared_ptr<MySql::Center_HornMessage> obj);
	~HornMessage();

public:
	HornMessageUuid get_horn_message_uuid() const;
	ItemId get_item_id() const;
	LanguageId get_language_id() const;
	std::uint64_t get_created_time() const;
	std::uint64_t get_expiry_time() const;
	AccountUuid get_from_account_uuid() const;

	const std::vector<std::pair<ChatMessageSlotId, std::string>> &get_segments() const {
		return m_segments;
	}

	void delete_from_game() noexcept;

	bool is_virtually_removed() const;
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

}

#endif
