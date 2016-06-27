#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../singletons/auction_center_map.hpp"
#include "../auction_center.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../msg/err_castle.hpp"
#include "../msg/err_item.hpp"

namespace EmperyCenter {

AUCTION_SERVLET("transfer/begin", root, session, params){
	const auto platform_id = boost::lexical_cast<PlatformId>(params.at("platform_id"));
	const auto &login_name = params.at("login_name");
	const auto map_object_uuid = MapObjectUuid(params.at("map_object_uuid"));
	const auto item_object = boost::lexical_cast<Poseidon::JsonObject>(params.at("items"));

	const auto account = AccountMap::get_or_reload_by_login_name(platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	const auto account_uuid = account->get_account_uuid();

	AccountMap::require_controller_token(account_uuid);

	const auto auction_center = AuctionCenterMap::require(account_uuid);
	const auto item_box = ItemBoxMap::require(account_uuid);

	auction_center->pump_transfer_status(map_object_uuid);

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}
	if(castle->get_owner_uuid() != account_uuid){
		return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
	}

	std::vector<AuctionCenter::TransferInfo> transfers;
	auction_center->get_transfer(transfers, map_object_uuid);
	if(!transfers.empty()){
		return Response(Msg::ERR_AUCTION_TRANSFER_IN_PROGRESS) <<transfers.front().due_time;
	}

	boost::container::flat_map<ItemId, std::uint64_t> items;
	items.reserve(item_object.size());
	for(auto it = item_object.begin(); it != item_object.end(); ++it){
		const auto count = static_cast<std::uint64_t>(it->second.get<double>());
		if(count == 0){
			continue;
		}
		items[boost::lexical_cast<ItemId>(it->first)] = count;
	}
	if(items.empty()){
		return Response(Msg::ERR_AUCTION_TRANSFER_EMPTY);
	}

	const auto result = auction_center->begin_transfer(castle, item_box, items);
	if(result.first){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<result.first;
	}
	if(result.second){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<result.second;
	}

	return Response();
}

}
