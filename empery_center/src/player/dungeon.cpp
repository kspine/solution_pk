#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/cs_dungeon.hpp"
#include "../msg/sc_dungeon.hpp"
#include "../msg/err_dungeon.hpp"
#include "../singletons/dungeon_box_map.hpp"
#include "../dungeon_box.hpp"
#include "../data/dungeon.hpp"
#include "../msg/err_item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_DungeonGetAll, account, session, /* req */){
	const auto dungeon_box = DungeonBoxMap::require(account->get_account_uuid());
	dungeon_box->pump_status();

	dungeon_box->synchronize_with_player(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_DungeonCreate, account, session, req){

	return Response();
}

}
