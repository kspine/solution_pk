#ifndef EMPERY_CENTER_FRIEND_BOX_HPP_
#define EMPERY_CENTER_FRIEND_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/fwd.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <vector>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Friend;
}

class PlayerSession;

class FriendBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum Category {
		CAT_DELETED     = 0,
		CAT_FRIEND      = 1,
		CAT_REQUESTING  = 2,
		CAT_REQUESTED   = 3,
		CAT_BLACKLIST   = 4,
	};

	struct FriendInfo {
		AccountUuid friend_uuid;
		Category category;
		std::string metadata;
		std::uint64_t updated_time;
	};

	struct AsyncRequestResultControl {
		boost::shared_ptr<Poseidon::JobPromise> promise;
		boost::shared_ptr<std::pair<long, std::string>> result;
	};

private:
	const AccountUuid m_account_uuid;

	boost::container::flat_map<Category,
		boost::container::flat_map<AccountUuid, boost::shared_ptr<MySql::Center_Friend>>> m_friends;

	boost::container::flat_map<Poseidon::Uuid, AsyncRequestResultControl> m_async_requests;

public:
	FriendBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_Friend>> &friends);
	~FriendBox();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	FriendInfo get(AccountUuid friend_uuid) const;
	void get_all(std::vector<FriendInfo> &ret) const;
	void get_by_category(std::vector<FriendInfo> &ret, Category category) const;
	void set(FriendInfo info);
	bool remove(AccountUuid friend_uuid) noexcept;

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;

	Poseidon::Uuid create_async_request(
		boost::shared_ptr<Poseidon::JobPromise> promise, boost::shared_ptr<std::pair<long, std::string>> result);
	bool set_async_request_result(Poseidon::Uuid transaction_uuid, std::pair<long, std::string> &&result);
	bool remove_async_request(Poseidon::Uuid transaction_uuid) noexcept;
};

inline void synchronize_friend_box_with_player(const boost::shared_ptr<const FriendBox> &friend_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	friend_box->synchronize_with_player(session);
}
inline void synchronize_friend_box_with_player(const boost::shared_ptr<FriendBox> &friend_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	friend_box->synchronize_with_player(session);
}

}

#endif
