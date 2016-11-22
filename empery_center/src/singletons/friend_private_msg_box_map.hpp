#ifndef EMPERY_CENTER_SINGLETONS_FRIEND_PRIVATE_MSG_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_FRIEND_PRIVATE_MSG_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class FriendPrivateMsgBox;
class FriendPrivateMsgData;

struct FriendPrivateMsgBoxMap {
	static void insert(AccountUuid account_uuid,AccountUuid friend_uuid,std::uint64_t utc_now,FriendPrivateMsgUuid msg_uuid,bool sender,bool read,bool deleted);
	static boost::shared_ptr<FriendPrivateMsgBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<FriendPrivateMsgBox> require(AccountUuid account_uuid);
	static void unload(AccountUuid account_uuid);
	
	static boost::shared_ptr<FriendPrivateMsgData> get_msg_data(FriendPrivateMsgUuid msg_uuid, LanguageId language_id);
	static boost::shared_ptr<FriendPrivateMsgData> require_msg_data(FriendPrivateMsgUuid msg_uuid, LanguageId language_id);
	static void insert_private_msg_data(boost::shared_ptr<FriendPrivateMsgData> private_msg_data);

private:
	FriendPrivateMsgBoxMap() = delete;
};

}

#endif
