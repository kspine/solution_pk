#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/err_account.hpp"
#include "../../../empery_center/src/msg/st_account.hpp"
#include "../../../empery_center/src/msg/ts_account.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"

namespace EmperyController {

CONTROLLER_SERVLET(Msg::ST_AccountAddItems, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::get_or_reload(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	Msg::TS_AccountAddItems msg;
	msg.account_uuid = account_uuid.str();
	msg.items.reserve(req.items.size());
	for(auto it = req.items.begin(); it != req.items.end(); ++it){
		auto &item = *msg.items.emplace(msg.items.end());
		item.item_id = it->item_id;
		item.count   = it->count;
		item.reason  = it->reason;
		item.param1  = it->param1;
		item.param2  = it->param2;
		item.param3  = it->param3;
	}

	auto using_controller = account->get_controller();
	if(!using_controller){
		using_controller = controller;
	}
	using_controller->send(msg);

	return Response();
}

}
