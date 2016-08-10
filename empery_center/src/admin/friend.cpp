#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../msg/err_friend.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/friend_box_map.hpp"
#include "../friend_box.hpp"

namespace EmperyCenter {

ADMIN_SERVLET("friend/get_all", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}
	AccountMap::require_controller_token(account_uuid, { });

	const auto friend_box = FriendBoxMap::require(account_uuid);

	std::vector<FriendBox::FriendInfo> friends;
	friend_box->get_all(friends);

	Poseidon::JsonArray elem_array;
	for(auto it = friends.begin(); it != friends.end(); ++it){
		Poseidon::JsonObject friend_object;
		friend_object[sslit("friend_uuid")]  = it->friend_uuid.str();
		friend_object[sslit("category")]     = static_cast<unsigned>(it->category);
		friend_object[sslit("metadata")]     = std::move(it->metadata);
		friend_object[sslit("updated_time")] = it->updated_time;
		elem_array.emplace_back(std::move(friend_object));
	}
	root[sslit("friends")] = std::move(elem_array);

	return Response();
}

ADMIN_SERVLET("friend/set", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));
	const auto friend_uuid  = AccountUuid(params.at("friend_uuid"));
	const auto category     = static_cast<FriendBox::Category>(boost::lexical_cast<unsigned>(params.at("category")));
	const auto &metadata    = params.get("metadata");

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}
	AccountMap::require_controller_token(account_uuid, { });

	const auto friend_box = FriendBoxMap::require(account_uuid);

	const auto utc_now = Poseidon::get_utc_time();

	FriendBox::FriendInfo info = { };
	info.friend_uuid  = friend_uuid;
	info.category     = category;
	info.metadata     = metadata;
	info.updated_time = utc_now;
	friend_box->set(std::move(info));

	return Response();
}

ADMIN_SERVLET("friend/remove", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));
	const auto friend_uuid  = AccountUuid(params.at("friend_uuid"));

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}
	AccountMap::require_controller_token(account_uuid, { });

	const auto friend_box = FriendBoxMap::require(account_uuid);

	if(!friend_box->remove(friend_uuid)){
		return Response(Msg::ERR_NO_SUCH_FRIEND) <<friend_uuid;
	}

	return Response();
}

}
