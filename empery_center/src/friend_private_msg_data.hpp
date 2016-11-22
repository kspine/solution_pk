#ifndef EMPERY_CENTER_FRIEND_PRIVATE_MSG_DATA_HPP_
#define EMPERY_CENTER_FRIEND_PRIVATE_MSG_DATA_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include <poseidon/cxx_util.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_FriendPrivateMsgData;
}

class PlayerSession;

class FriendPrivateMsgData : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const boost::shared_ptr<MySql::Center_FriendPrivateMsgData> m_obj;

	std::vector<std::pair<ChatMessageSlotId, std::string>> m_segments;
	boost::container::flat_map<ItemId, std::uint64_t> m_attachments;

public:
	FriendPrivateMsgData(FriendPrivateMsgUuid msg_uuid, LanguageId language_id, std::uint64_t created_time,AccountUuid from_account_uuid,std::vector<std::pair<ChatMessageSlotId, std::string>> segments);
	explicit FriendPrivateMsgData(boost::shared_ptr<MySql::Center_FriendPrivateMsgData> obj);
	~FriendPrivateMsgData();

public:
	FriendPrivateMsgUuid get_msg_uuid() const;
	LanguageId get_language_id() const;
	std::uint64_t get_created_time() const;

	AccountUuid get_from_account_uuid() const;
	void set_from_account_uuid(AccountUuid account_uuid);

	const std::vector<std::pair<ChatMessageSlotId, std::string>> &get_segments() const;
	void set_segments(std::vector<std::pair<ChatMessageSlotId, std::string>> segments);

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

}

#endif
