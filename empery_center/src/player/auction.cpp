#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_auction.hpp"
#include "../singletons/auction_center_map.hpp"
#include "../auction_center.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_AuctionQueryStatus, account, session, /* req */){
	const auto auction_center = AuctionCenterMap::require(account->get_account_uuid());
	auction_center->pump_status();

	auction_center->synchronize_with_player(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AuctionUpdateTransferStatus, account, session, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);

	const auto auction_center = AuctionCenterMap::require(account->get_account_uuid());
	auction_center->pump_transfer_status(map_object_uuid);

	auction_center->synchronize_transfer_with_player(map_object_uuid, session);

	return Response();
}

}
