#ifndef EMPERY_CENTER_SINGLETONS_FRIEND_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_FRIEND_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class FriendBox;

struct FriendBoxMap {
	static boost::shared_ptr<FriendBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<FriendBox> require(AccountUuid account_uuid);
	static void unload(AccountUuid account_uuid);

	static void random(std::vector<AccountUuid> &ret, std::size_t max_count, const boost::shared_ptr<FriendBox> &excluding_box);

private:
	FriendBoxMap() = delete;
};

}

#endif
