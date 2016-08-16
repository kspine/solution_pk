#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sl_castle.hpp"
#include "../mysql/resource_changed.hpp"
#include "../mysql/battalion_changed.hpp"
#include "../mysql/wounded_soldier_changed.hpp"
#include "../mysql/castle_protection.hpp"

namespace EmperyCenterLog {

LOG_SERVLET(Msg::SL_CastleResourceChanged, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_ResourceChanged>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.map_object_uuid), Poseidon::Uuid(req.owner_uuid), req.resource_id, req.old_amount, req.new_amount,
		static_cast<std::int64_t>(req.new_amount - req.old_amount), req.reason, req.param1, req.param2, req.param3);
	obj->async_save(false, true);
}

LOG_SERVLET(Msg::SL_CastleSoldierChanged, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_BattalionChanged>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.map_object_uuid), Poseidon::Uuid(req.owner_uuid), req.map_object_type_id, req.old_count, req.new_count,
		static_cast<std::int64_t>(req.new_count - req.old_count), req.reason, req.param1, req.param2, req.param3);
	obj->async_save(false, true);
}

LOG_SERVLET(Msg::SL_CastleWoundedSoldierChanged, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_WoundedSoldierChanged>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.map_object_uuid), Poseidon::Uuid(req.owner_uuid), req.map_object_type_id, req.old_count, req.new_count,
		static_cast<std::int64_t>(req.new_count - req.old_count), req.reason, req.param1, req.param2, req.param3);
	obj->async_save(false, true);
}

LOG_SERVLET(Msg::SL_CastleProtection, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_CastleProtection>(
		Poseidon::get_utc_time(),
		Poseidon::Uuid(req.map_object_uuid), Poseidon::Uuid(req.owner_uuid),
		req.delta_preparation_duration, req.delta_protection_duration, req.protection_end);
	obj->async_save(false, true);
}

}
