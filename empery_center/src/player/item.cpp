#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_item.hpp"
#include "../msg/sc_item.hpp"
#include "../msg/cerr_item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_ItemGetAllItems, accountUuid, session, /* req */){
	const auto itemBox = ItemBoxMap::require(accountUuid);
	itemBox->pumpStatus(true);

	return Response();
}

}
