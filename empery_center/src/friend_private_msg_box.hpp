#ifndef EMPERY_CENTER_FRIEND_PRIVATE_MSG_BOX_HPP_
#define EMPERY_CENTER_FRIEND_PRIVATE_MSG_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <deque>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_FriendPrivateMsgRecent;
}

class FriendPrivateMsgBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	struct PrivateMsgInfo {
		Poseidon::Uuid        auto_uuid;
		std::uint64_t         timestamp;
		AccountUuid           friend_account_uuid;
		FriendPrivateMsgUuid  msg_uuid; 
		bool                  sender;
		bool                  read;
		bool                  deleted;
	};

private:
	const AccountUuid m_account_uuid;

	std::deque<boost::shared_ptr<MySql::Center_FriendPrivateMsgRecent>> m_recent_msgs;

public:
	FriendPrivateMsgBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_FriendPrivateMsgRecent>> &recent_msgs);
	~FriendPrivateMsgBox();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	void get_all(std::vector<PrivateMsgInfo> &ret) const;
	void get_offline(std::vector<PrivateMsgInfo> &ret) const;
	void get_recent_contact(boost::container::flat_map<AccountUuid, std::uint64_t> &recent_contact) const;

	void push(std::uint64_t timestamp,AccountUuid friend_account_uuid,FriendPrivateMsgUuid msg_uuid,bool sender,bool read,bool deleted);
};

}

#endif
