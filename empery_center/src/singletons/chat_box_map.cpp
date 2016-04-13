#include "../precompiled.hpp"
#include "chat_box_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../chat_box.hpp"
#include "account_map.hpp"

namespace EmperyCenter {

namespace {
	struct ChatBoxElement {
		AccountUuid account_uuid;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<ChatBox> chat_box;

		ChatBoxElement(AccountUuid account_uuid_, std::uint64_t unload_time_)
			: account_uuid(account_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(ChatBoxContainer, ChatBoxElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<ChatBoxContainer> g_chat_box_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Chat box gc timer: now = ", now);

		const auto chat_box_map = g_chat_box_map.lock();
		if(!chat_box_map){
			return;
		}

		for(;;){
			const auto it = chat_box_map->begin<1>();
			if(it == chat_box_map->end<1>()){
				break;
			}
			if(now < it->unload_time){
				break;
			}

			if(it->chat_box.use_count() > 1){
				chat_box_map->set_key<1, 1>(it, now + 1000);
			} else {
				LOG_EMPERY_CENTER_DEBUG("Reclaiming chat box: account_uuid = ", it->account_uuid);
				chat_box_map->erase<1>(it);
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto chat_box_map = boost::make_shared<ChatBoxContainer>();
		g_chat_box_map = chat_box_map;
		handles.push(chat_box_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<ChatBox> ChatBoxMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto chat_box_map = g_chat_box_map.lock();
	if(!chat_box_map){
		LOG_EMPERY_CENTER_WARNING("ChatBoxMap is not loaded.");
		return { };
	}

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_DEBUG("Account not found: account_uuid = ", account_uuid);
		return { };
	}

	auto it = chat_box_map->find<0>(account_uuid);
	if(it == chat_box_map->end<0>()){
		it = chat_box_map->insert<0>(it, ChatBoxElement(account_uuid, 0));
	}
	if(!it->chat_box){
		LOG_EMPERY_CENTER_DEBUG("Loading chat box: account_uuid = ", account_uuid);

		auto chat_box = boost::make_shared<ChatBox>(account_uuid);

		it->chat_box = std::move(chat_box);

		assert(it->chat_box);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	chat_box_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->chat_box;
}
boost::shared_ptr<ChatBox> ChatBoxMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto ret = get(account_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Chat box not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Chat box not found"));
	}
	return ret;
}

}
