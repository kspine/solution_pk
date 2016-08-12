#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sl_auction.hpp"
#include "../mysql/auction_item_changed.hpp"

namespace EmperyCenterLog {

LOG_SERVLET(Msg::SL_AuctionItemChanged, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_AuctionItemChanged>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.account_uuid), req.item_id, req.old_count, req.new_count,
		static_cast<std::int64_t>(req.new_count - req.old_count), req.reason, req.param1, req.param2, req.param3);
	obj->async_save(false, true);
}

}
