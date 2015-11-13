#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_item.hpp"
#include "../msg/sc_item.hpp"
#include "../msg/cerr_item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_ItemGetAllItems, account_uuid, session, /* req */){
	const auto item_box = ItemBoxMap::require(account_uuid);
	item_box->pump_status(true);

	return Response();
}

}
