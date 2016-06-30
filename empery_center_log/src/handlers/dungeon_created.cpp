#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/dungeon_created.hpp"
#include "../../../empery_center/src/events/dungeon.hpp"

namespace EmperyCenterLog {

MODULE_RAII_PRIORITY(handles, 5000){
	auto listener = Poseidon::EventDispatcher::register_listener<Events::DungeonCreated>(
		[](const boost::shared_ptr<Events::DungeonCreated> &event){
			const auto obj = boost::make_shared<MySql::CenterLog_DungeonCreated>(Poseidon::get_utc_time(),
			event->account_uuid.get(), event->dungeon_type_id.get(), event->old_score);
			obj->async_save(false, true);
		});
	LOG_EMPERY_CENTER_LOG_DEBUG("Created DungeonCreated listener");
	handles.push(std::move(listener));
}

}
